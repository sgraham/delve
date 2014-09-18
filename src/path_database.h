// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// The ChangeJournal stores the raw FRN (longlong) to the parent directory,
// but there's no fast, simple way to map that back to its path (especially
// during deletion updates), so we must maintain a database of paths here.

#ifndef DELVE_PATH_DATABASE_H_
#define DELVE_PATH_DATABASE_H_

#include <map>
#include <string>
#include <vector>
#include <windows.h>
using namespace std;

struct PathDbEntry {
  DWORDLONG parent_frn;
  wstring name;
};

class PathDatabase {
public:
  PathDatabase();

  // Files all directories on the volume specified by |drive_letter| by
  // reading the MFT. Optionally, restricted to range base on USN.
  void PopulateFromMft(wchar_t drive_letter,
                       DWORDLONG usn_from = 0,
                       DWORDLONG usn_to = ULLONG_MAX);

  bool LoadFrom(const wstring& filename, string* error);
  bool SaveTo(const wstring& filename, string* error) const;

  void Set(DWORDLONG index, const wstring& name, DWORDLONG parent_index);
  bool Get(DWORDLONG index, PathDbEntry* entry) const;
  void Remove(DWORDLONG index);
  bool GetPath(DWORDLONG index, wstring* path);

  size_t NumEntries() const;
  void SetLastUsn(DWORDLONG usn) { last_usn_ = usn; }
  DWORDLONG LastUsn() const { return last_usn_; }
  DWORDLONG UsnJournalId() const { return usn_journal_id_; }

private:
  map<DWORDLONG, PathDbEntry> data_;
  typedef map<DWORDLONG, PathDbEntry>::const_iterator DataI;

  // Last USN retrieved either via MFT population or as processed by change
  // journal updates.
  DWORDLONG last_usn_;

  // USN Journal ID that this database was built from.
  DWORDLONG usn_journal_id_;
};

#endif  // DELVE_PATH_DATABASE_H_
