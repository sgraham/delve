// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "change_journal.h"

#include "file_extra_util.h"
#include "path_database.h"
#include "util.h"

#include <assert.h>

namespace {

string GetReasonString(DWORD reason) {
  static const char* reasons[] = {
    "DataOverwrite",         // 0x00000001
    "DataExtend",            // 0x00000002
    "DataTruncation",        // 0x00000004
    "0x00000008",            // 0x00000008
    "NamedDataOverwrite",    // 0x00000010
    "NamedDataExtend",       // 0x00000020
    "NamedDataTruncation",   // 0x00000040
    "0x00000080",            // 0x00000080
    "FileCreate",            // 0x00000100
    "FileDelete",            // 0x00000200
    "PropertyChange",        // 0x00000400
    "SecurityChange",        // 0x00000800
    "RenameOldName",         // 0x00001000
    "RenameNewName",         // 0x00002000
    "IndexableChange",       // 0x00004000
    "BasicInfoChange",       // 0x00008000
    "HardLinkChange",        // 0x00010000
    "CompressionChange",     // 0x00020000
    "EncryptionChange",      // 0x00040000
    "ObjectIdChange",        // 0x00080000
    "ReparsePointChange",    // 0x00100000
    "StreamChange",          // 0x00200000
    "0x00400000",            // 0x00400000
    "0x00800000",            // 0x00800000
    "0x01000000",            // 0x01000000
    "0x02000000",            // 0x02000000
    "0x04000000",            // 0x04000000
    "0x08000000",            // 0x08000000
    "0x10000000",            // 0x10000000
    "0x20000000",            // 0x20000000
    "0x40000000",            // 0x40000000
    "*Close*"                // 0x80000000
  };

  string ret;
  for (DWORD i = 0; reason != 0; reason >>= 1, ++i) {
    if (reason & 1) {
      if (ret != "")
        ret += ", ";
      ret += reasons[i];
    }
  }
  return ret;
}

}  // namespace

ChangeJournal::ChangeJournal(wchar_t drive_letter, PathDatabase& path_database) :
    drive_letter_(drive_letter),
    path_database_(path_database),
    change_delegate_(NULL) {
  assert(toupper(drive_letter) == drive_letter);
  // This assert is a bit annoying for testing with a faked PathDatabase.
  //PathDbEntry root;
  //assert((path_database_.Get(0, &root), root.name[0] == drive_letter));
  cj_sync_ = OpenVolume(drive_letter, false);
  cj_async_ = OpenVolume(drive_letter, true);
  if (cj_async_ == INVALID_HANDLE_VALUE)
    Win32Fatal("Open async");
  memset(&cj_async_overlapped_, 0, sizeof(cj_async_overlapped_));
  cj_async_overlapped_.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  if (!cj_async_overlapped_.hEvent)
    Win32Fatal("CreateEvent");

  rujd_.StartUsn = path_database_.LastUsn();
  rujd_.ReasonMask = 0xffffffff;
  rujd_.ReturnOnlyOnClose = false;
  rujd_.Timeout = 0;
  rujd_.BytesToWaitFor = 0;
  rujd_.UsnJournalID = path_database_.UsnJournalId();
  memset(cj_data_, 0, sizeof(cj_data_));
  valid_cj_data_bytes_ = 0;
  usn_record_ = NULL;
}

ChangeJournal::~ChangeJournal() {
  CloseHandle(cj_sync_);
  CloseHandle(cj_async_);
  SetEvent(cj_async_overlapped_.hEvent);
  CloseHandle(cj_async_overlapped_.hEvent);
}

void ChangeJournal::SetChangeNotificationDelegate(
    ChangeNotificationDelegate* change_delegate) {
  change_delegate_ = change_delegate;
}

void ChangeJournal::ProcessAvailableRecords() {
  if (!change_delegate_)
    Warning("No delegate specified before WatchIteration.");
  ReadJournalData();
}

// TODO: Shutdown behavior.
void ChangeJournal::WatchLoop() {
  for (;;) {
    ProcessAvailableRecords();
    WaitForSingleObject(cj_async_overlapped_.hEvent, INFINITE);
  }
}

USN_RECORD* ChangeJournal::MoveToNext(bool* err) {
  *err = false;
  if (usn_record_ == NULL ||
      reinterpret_cast<BYTE*>(usn_record_) + usn_record_->RecordLength >=
          cj_data_ + valid_cj_data_bytes_) {
    usn_record_ = NULL;
    BOOL success = DeviceIoControl(
        cj_sync_, FSCTL_READ_USN_JOURNAL, &rujd_, sizeof(rujd_), cj_data_,
        sizeof(cj_data_), &valid_cj_data_bytes_, NULL);
    if (success) {
      rujd_.StartUsn = *reinterpret_cast<USN*>(cj_data_);
      if (valid_cj_data_bytes_ > sizeof(USN)) {
        usn_record_ = reinterpret_cast<USN_RECORD*>(&cj_data_[sizeof(USN)]);
      }
    } else {
      // Some check has failed. Records overflow, USN deleted, etc.
      // Cache needs to be fully flushed, as we can't trust any of it
      // now.
      DWORD failure = GetLastError();
      // Possible errors:
      // - ERROR_JOURNAL_DELETE_IN_PROGRESS
      // - ERROR_JOURNAL_NOT_ACTIVE
      // - ERROR_INVALID_PARAMETER
      // - ERROR_JOURNAL_ENTRY_DELETED
      Warning("DeviceIoControl failed: GLE=%d", failure);
      *err = true;
    }
  } else {
    usn_record_ = reinterpret_cast<USN_RECORD*>(
        reinterpret_cast<BYTE*>(usn_record_) + usn_record_->RecordLength);
  }
  return usn_record_;
}

bool ChangeJournal::SetUpNotification() {
  READ_USN_JOURNAL_DATA rujd;
  rujd = rujd_;
  rujd.BytesToWaitFor = 1;
  bool success = DeviceIoControl(cj_async_, FSCTL_READ_USN_JOURNAL,
      &rujd, sizeof(rujd), &usn_async_, sizeof(usn_async_), NULL,
      &cj_async_overlapped_);
  return !success && GetLastError() != ERROR_IO_PENDING;
}

// TODO: Wide/narrow/utf8 is a total mess.
bool ChangeJournal::ReadJournalData() {
  for (;;) {
    USN_RECORD* record;
    bool err = false;
    for (;;) {
      record = MoveToNext(&err);
      if (!record)
        break;
      wstring wide(reinterpret_cast<wchar_t*>(
                      reinterpret_cast<BYTE*>(record) + record->FileNameOffset),
                  record->FileNameLength / sizeof(WCHAR));

      // If something's happening to a directory, we need to update the path
      // database.
      if ((record->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
          (record->Reason & USN_REASON_CLOSE)) {
        if ((record->Reason & USN_REASON_FILE_CREATE) ||
            (record->Reason & USN_REASON_RENAME_NEW_NAME)) {
          path_database_.Set(record->FileReferenceNumber, wide,
                             record->ParentFileReferenceNumber);
        }
        if (record->Reason & USN_REASON_FILE_DELETE)
          path_database_.Remove(record->FileReferenceNumber);
      }

      if (record->Reason & USN_REASON_HARD_LINK_CHANGE) {
        // The name we receive in this notification is the target, but there's
        // no information about any of the links. So, use
        // FindFirst/NextFileNameW to walk all the hard links to this file,
        // and notify about all of them.
        wstring path;
        if (path_database_.GetPath(record->ParentFileReferenceNumber, &path)) {
          wstring full_name = path + L"\\" + wide;
          wchar_t other[_MAX_PATH];
          DWORD len = sizeof(other);
          wstring wide_name(full_name.begin(), full_name.end());
          HANDLE handle = FindFirstFileNameW(wide_name.c_str(), 0, &len, other);
          // Not finding shouldn't be fatal. Could create/modify and then
          // remove before we process this record.
          if (handle != INVALID_HANDLE_VALUE) {
            for (;;) {
              if (change_delegate_)
                change_delegate_->FileChanged(full_name, record->Reason);
              fprintf(stderr, "hardlink: %s", full_name.c_str());
              BOOL success = FindNextFileNameW(handle, &len, other);
              if (!success)
                break;
            }
            FindClose(handle);
          }
        }
      }

      // TODO: Culling of useless/redundant files.
      // TODO: Cull stuff that doesn't live inside our interesting roots.

      wstring path;
      if (path_database_.GetPath(record->ParentFileReferenceNumber, &path)) {
        wstring full_name = path + L"\\" + wide;
        if (change_delegate_)
          change_delegate_->FileChanged(full_name, record->Reason);
      } else {
        // Can happen if the parent directory is removed before we
        // process this record, if we don't have access to it, etc.
        Warning("error for %llx\n", record->ParentFileReferenceNumber);
        err = true;
      }

      path_database_.SetLastUsn(record->Usn);
    }

    if (err) {
      // Something bad happened: maybe the journal overflowed, didn't exist,
      // etc. Try starting over.
      Warning("error from path database or change journal, refreshing.");
      return false;
    }

    // Normally, we'll break here. If we fail to set up async
    // notification though, just try to process again because more
    // data may have been received.
    if (SetUpNotification())
      break;
  }
  return true;
}
