// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "path_database.h"

#include <direct.h>

#include "file_extra_util.h"
#include "test.h"

TEST(PathDatabaseTest, Empty) {
  PathDatabase db;
  EXPECT_EQ(0, db.NumEntries());
}

TEST(PathDatabaseTest, Basic) {
  PathDatabase db;
  db.Set(235, L"stuffy", 42);
  db.Set(42, L"things", 40);
  EXPECT_EQ(2, db.NumEntries());

  PathDbEntry into;

  EXPECT_EQ(true, db.Get(235, &into));
  EXPECT_EQ(into.name, L"stuffy");
  EXPECT_EQ(into.parent_frn, 42);

  EXPECT_EQ(true, db.Get(42, &into));
  EXPECT_EQ(into.name, L"things");
  EXPECT_EQ(into.parent_frn, 40);

  EXPECT_EQ(2, db.NumEntries());
}

TEST(PathDatabaseTest, Replace) {
  PathDatabase db;
  db.Set(235, L"stuffy", 42);
  db.Set(42, L"things", 40);
  EXPECT_EQ(2, db.NumEntries());
  db.Set(42, L"blah", 244);
  EXPECT_EQ(2, db.NumEntries());

  PathDbEntry into;

  EXPECT_EQ(true, db.Get(42, &into));
  EXPECT_EQ(into.name, L"blah");
  EXPECT_EQ(into.parent_frn, 244);
}

TEST(PathDatabaseTest, SaveAndLoad) {
  ScopedTempDir temp;
  temp.CreateAndEnter("pathdb-save-load");

  string error;
  {
    PathDatabase db;
    db.Set(235, L"stuffy", 42);
    db.Set(42, L"things", 40);
    db.Set(999, L"we are a path", 10203);
    EXPECT_EQ(3, db.NumEntries());
    EXPECT_EQ(true, db.SaveTo(L"my-db", &error));
  }

  {
    PathDatabase db;
    EXPECT_EQ(true, db.LoadFrom(L"my-db", &error));
    EXPECT_EQ(3, db.NumEntries());

    PathDbEntry into;

    EXPECT_EQ(true, db.Get(235, &into));
    EXPECT_EQ(into.name, L"stuffy");
    EXPECT_EQ(into.parent_frn, 42);

    EXPECT_EQ(true, db.Get(42, &into));
    EXPECT_EQ(into.name, L"things");
    EXPECT_EQ(into.parent_frn, 40);

    EXPECT_EQ(true, db.Get(999, &into));
    EXPECT_EQ(into.name, L"we are a path");
    EXPECT_EQ(into.parent_frn, 10203);
  }


  temp.Cleanup();
}


namespace {

DWORDLONG GetFrnOfPath(const wstring& path) {
  HANDLE dir = ::CreateFileW(path.c_str(),
                             0,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_FLAG_BACKUP_SEMANTICS,
                             NULL);
  BY_HANDLE_FILE_INFORMATION bhfi;
  GetFileInformationByHandle(dir, &bhfi);
  CloseHandle(dir);
  DWORDLONG frn = static_cast<DWORDLONG>(bhfi.nFileIndexHigh) << 32 |
                  static_cast<DWORDLONG>(bhfi.nFileIndexLow);
  return frn;
}

}  // namespace

TEST(PathDatabaseTest, PopulateViaMftFull) {
  ScopedTempDir temp;
  temp.CreateAndEnter("populate-via-mft");

  wstring parent_dir = L"parent";
  _wmkdir(parent_dir.c_str());
  DWORDLONG parent_dir_frn = GetFrnOfPath(parent_dir);

  PathDatabase before;
  before.PopulateFromMft(GetCurrentVolume());

  wstring child_dir = L"child";
  _wchdir(parent_dir.c_str());
  _wmkdir(child_dir.c_str());

  DWORDLONG dir_frn = GetFrnOfPath(child_dir);

  PathDatabase after;
  after.PopulateFromMft(GetCurrentVolume());

  PathDbEntry path_entry;

  EXPECT_EQ(true, before.Get(parent_dir_frn, &path_entry));
  EXPECT_EQ(false, before.Get(dir_frn, &path_entry));

  EXPECT_EQ(true, after.Get(dir_frn, &path_entry));
  EXPECT_EQ(parent_dir_frn, path_entry.parent_frn);

  temp.Cleanup();
}

TEST(PathDatabaseTest, PopulateViaMftIncremental) {
  ScopedTempDir temp;
  temp.CreateAndEnter("populate-incremental");

  PathDatabase before;
  before.PopulateFromMft(GetCurrentVolume());

  _mkdir("subdir");

  PathDatabase after;
  after.PopulateFromMft(GetCurrentVolume(), before.LastUsn());

  // One for root which is always there, plus the one we made.
  // TODO: This is flaky if the machine is busy and creates a directory.
  EXPECT_EQ(2, after.NumEntries());

  temp.Cleanup();
}
