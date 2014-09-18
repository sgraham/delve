// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "path_database.h"

#include "file_extra_util.h"
#include "line_reader.h"
#include "util.h"

#include <limits.h>

namespace {

const wchar_t kFileSignature[] = L"# path database v%d\n";
const int kCurrentVersion = 1;
const wchar_t kLastUsnFormat[] = L"last usn %lld\n";
const wchar_t kUsnJournalIdFormat[] = L"journal id %lld\n";

void QueryUsnJournal(HANDLE volume, USN_JOURNAL_DATA* usn) {
  DWORD cb;
  bool success = DeviceIoControl(volume, FSCTL_QUERY_USN_JOURNAL, NULL, 0,
                                 usn, sizeof(*usn), &cb, NULL);
  if (!success)
    Win32Fatal("DeviceIoControl, ChangeJournal::Query");
}

}

PathDatabase::PathDatabase() : last_usn_(0), usn_journal_id_(0) {
}

// Hmm, this is actually faster than I thought. It's ~800ms on a not too big
// SSD, a couple seconds on medium sized laptop hard drive. If there's going
// to be a daemon anyway, then persisting it might be unnecessary. It's about
// 8 sec on a 1TB spinning disk when the cache is hot.
void PathDatabase::PopulateFromMft(
    wchar_t drive_letter, DWORDLONG usn_from, DWORDLONG usn_to) {
  HANDLE volume = OpenVolume(drive_letter, false);

  USN_JOURNAL_DATA ujd;
  QueryUsnJournal(volume, &ujd);
  if (usn_to == ULLONG_MAX)
    usn_to = ujd.NextUsn;
  usn_journal_id_ = ujd.UsnJournalID;

  data_.clear();

  // Get the FRN of the root of drive.
  wstring root(L"?:\\");
  root[0] = drive_letter;
  HANDLE dir = ::CreateFileW(root.c_str(),
                             0,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_FLAG_BACKUP_SEMANTICS,
                             NULL);
  BY_HANDLE_FILE_INFORMATION bhfi;
  GetFileInformationByHandle(dir, &bhfi);
  CloseHandle(dir);
  DWORDLONG root_index = static_cast<DWORDLONG>(bhfi.nFileIndexHigh) << 32 |
                         static_cast<DWORDLONG>(bhfi.nFileIndexLow);
  wstring drive_name(L"?:");
  drive_name[0] = drive_letter;
  Set(root_index, drive_name, 0);

  // Use the MFT to enumerate the rest of the disk. Enumerating from 0 to
  // NextUsn will enumerate every file and directory on the volume.
  MFT_ENUM_DATA med;
  med.StartFileReferenceNumber = 0;
  med.LowUsn = usn_from;
  med.HighUsn = usn_to;

  const size_t kBufferSize = 1024 << 10;
  BYTE* data = new BYTE[kBufferSize];
  DWORD bytes_read;
  DWORD total_bytes_read = 0;
  int count = 0;
  while (DeviceIoControl(volume, FSCTL_ENUM_USN_DATA, &med, sizeof(med),
                         data, kBufferSize, &bytes_read, NULL)) {
    USN_RECORD* record = reinterpret_cast<USN_RECORD*>(&data[sizeof(USN)]);
    while (reinterpret_cast<BYTE*>(record) < data + bytes_read) {
      if (record->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        BYTE* start = reinterpret_cast<BYTE*>(record) + record->FileNameOffset;
        wstring wide(reinterpret_cast<wchar_t*>(start),
                     record->FileNameLength / sizeof(WCHAR));
        Set(record->FileReferenceNumber, wide,
            record->ParentFileReferenceNumber);
      }
      record = reinterpret_cast<USN_RECORD*>(
          reinterpret_cast<BYTE*>(record) + record->RecordLength);
    }
    med.StartFileReferenceNumber = *reinterpret_cast<DWORDLONG*>(data);
    total_bytes_read += bytes_read;
    count++;
  }
  delete [] data;

  last_usn_ = usn_to;

  CloseHandle(volume);
  //fprintf(stderr, "BuildPathDatabase, %d bytes in %d loops\n",
  //        total_bytes_read, count);
}

bool PathDatabase::LoadFrom(const wstring& filename, string* error) {
  FILE* f = _wfopen(filename.c_str(), L"rb");
  if (!f) {
    *error = "fopen f";
    return false;
  }
  data_.clear();
  LineReader reader(f);
  wchar_t* line_start = NULL;
  wchar_t* line_end = NULL;
  int log_version = 0;
  last_usn_ = ULLONG_MAX;
  usn_journal_id_ = ULLONG_MAX;
  while (reader.ReadLine(&line_start, &line_end)) {
    if (!log_version) {
      swscanf(line_start, kFileSignature, &log_version);
      continue;
    }

    if (last_usn_ == ULLONG_MAX) {
      swscanf(line_start, kLastUsnFormat, &last_usn_);
      continue;
    }

    if (usn_journal_id_ == ULLONG_MAX) {
      swscanf(line_start, kUsnJournalIdFormat, &usn_journal_id_);
      continue;
    }

    // No newline? Read the next chunk.
    if (!line_end)
      continue;

    const wchar_t kFieldSeparator = L'\t';

    wchar_t* start = line_start;
    wchar_t* end = reinterpret_cast<wchar_t*>(
        wmemchr(start, kFieldSeparator, line_end - start));
    if (!end)
      continue;
    *end = 0;

    DWORDLONG index = _wcstoui64(start, NULL, 10);
    start = end + 1;

    end = reinterpret_cast<wchar_t*>(
        wmemchr(start, kFieldSeparator, line_end - start));
    if (!end)
      continue;
    *end = 0;
    DWORDLONG parent_index = _wcstoui64(start, NULL, 10);
    start = end + 1;

    *line_end = 0;
    Set(index, start, parent_index);
  }
  fclose(f);

  return true;
}

bool PathDatabase::SaveTo(const wstring& filename, string* error) const {
  // ReplaceFile doesn't succeed if the file doesn't exist. So open for append
  // so that the rest of the code doesn't change below.
  FILE* ensure_exists = _wfopen(filename.c_str(), L"ab");
  if (!ensure_exists) {
    *error = "fopen ensure_exists";
    return false;
  }
  fclose(ensure_exists);

  wstring temp_name = filename + L".saving";
  FILE* f = _wfopen(temp_name.c_str(), L"wb");
  if (!f) {
    *error = "fopen f";
    return false;
  }
  fwprintf(f, kFileSignature, kCurrentVersion);
  fwprintf(f, kLastUsnFormat, last_usn_);
  fwprintf(f, kUsnJournalIdFormat, usn_journal_id_);
  for (DataI i(data_.begin()); i != data_.end(); ++i) {
    fwprintf(f, L"%lld\t%lld\t%s\n",
        i->first, i->second.parent_frn, i->second.name.c_str());
  }
  fclose(f);
  if (!::ReplaceFileW(
           filename.c_str(), temp_name.c_str(), NULL, 0, NULL, NULL)) {
    *error = "ReplaceFile";
    return false;
  }
  return true;
}

void PathDatabase::Set(DWORDLONG index,
                       const wstring& name,
                       DWORDLONG parent_index) {
  PathDbEntry item = { parent_index, name };
  data_[index] = item;
}

bool PathDatabase::Get(DWORDLONG index, PathDbEntry* entry) const {
  DataI i = data_.find(index);
  if (i != data_.end()) {
    *entry = i->second;
    return true;
  }
  return false;
}

void PathDatabase::Remove(DWORDLONG index) {
  data_.erase(index);
}

bool PathDatabase::GetPath(DWORDLONG index, wstring* path) {
  wstring full;
  do {
    DataI i = data_.find(index);
    if (i == data_.end())
      return false;
    full = i->second.name + (full.size() > 0 ? L"\\" : L"") + full;
    index = i->second.parent_frn;
  } while (index != 0);
  *path = full;
  return true;
}

size_t PathDatabase::NumEntries() const {
  return data_.size();
}
