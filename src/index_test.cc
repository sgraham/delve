// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "test.h"

#include "index.h"

TEST(Index, ReadSimple) {
  Index index("src/index_test_data");
  EXPECT_EQ("dir1/subdir2/file.c", string(index.NameBytes(0)));
  EXPECT_EQ("dir1/xxx/somefile.h", string(index.NameBytes(1)));
}
