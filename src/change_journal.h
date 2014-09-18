// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DELVE_CHANGE_JOURNAL_H_
#define DELVE_CHANGE_JOURNAL_H_

#include <windows.h>
#include <string>
using namespace std;

#include "util.h"

class PathDatabase;

class ChangeNotificationDelegate {
public:
  virtual ~ChangeNotificationDelegate() {};
  virtual void FileChanged(const wstring& full_path, DWORD reason_flags) = 0;
};

class ChangeJournal {
public:
  ChangeJournal(char drive_letter, PathDatabase& path_database);
  ~ChangeJournal();

  void SetChangeNotificationDelegate(
      ChangeNotificationDelegate* change_delegate);

  void ProcessAvailableRecords();
  void WatchLoop();

private:

  // Process all available records, advancing by using MoveToNext.
  bool ReadJournalData();

  // Advance to the next record in the stream. Updates and returns
  // |usn_record_|.
  USN_RECORD* MoveToNext(bool* err);

  // Queue up read of journal data. Will return false on failure which either
  // means there was more data to be read or it failed for some other reason.
  // In either case, we'll try to read more data, and then attempt again.
  bool SetUpNotification();

  char drive_letter_;
  PathDatabase& path_database_;

  // Where we should send information about changes.
  ChangeNotificationDelegate* change_delegate_;

  // Handle to volume, opened as sync. Used to do main read of data.
  HANDLE cj_sync_;

  // Async reading used for notification of new data.
  // Handle to volume, opened as async.
  HANDLE cj_async_;

  // Overlapped structure for async read.
  OVERLAPPED cj_async_overlapped_;

  // Parameters for reading journal.
  READ_USN_JOURNAL_DATA rujd_;

  // Buffer of read data.
  BYTE cj_data_[32768];

  // Number of valid bytes in cj_data_.
  DWORD valid_cj_data_bytes_;

  // Pointer to current record.
  USN_RECORD* usn_record_;

  // Read buffer for async read to target (not used after reading).
  USN usn_async_;

  DISALLOW_COPY_AND_ASSIGN(ChangeJournal);
};

#endif  // DELVE_CHANGE_JOURNAL_H_
