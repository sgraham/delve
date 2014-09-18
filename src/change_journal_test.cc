// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "change_journal.h"

#include "file_extra_util.h"
#include "path_database.h"
#include "test.h"

#include <set>

using namespace std;

namespace {

class Notifier : public ChangeNotificationDelegate {
 public:
  virtual void FileChanged(const wstring& full_path,
                           DWORD reason_flags) override {
    (void)reason_flags;
    full_paths.insert(full_path);
  }
  set<wstring> full_paths;
};

struct ChangeJournalTest : public testing::Test {
  Notifier notifier;
};

}  // namespace

TEST_F(ChangeJournalTest, Basic) {
  PathDatabase db;
  db.PopulateFromMft(::GetCurrentVolume());

  ChangeJournal cj(::GetCurrentVolume(), db);
  cj.SetChangeNotificationDelegate(&notifier);

  ScopedTempDir temp;
  temp.CreateAndEnter("DirCreated");

  cj.ProcessAvailableRecords();

  bool found_dir_in_changes = false;
  for (set<wstring>::const_iterator i(notifier.full_paths.begin());
       i != notifier.full_paths.end();
       ++i) {
    if (i->find(L"DirCreated") != string::npos) {
      found_dir_in_changes = true;
      break;
    }
  }
  EXPECT_EQ(true, found_dir_in_changes);

  temp.Cleanup();
}
