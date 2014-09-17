// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "memory_mapped_file.h"

#include "test.h"

TEST(MemoryMappedFileTest, Simple) {
  MemoryMappedFile test("src/mmap_test");
  EXPECT_EQ(9, test.Size());
  EXPECT_EQ('a', test.Data()[0]);
  EXPECT_EQ('b', test.Data()[1]);
  EXPECT_EQ('c', test.Data()[2]);
  EXPECT_EQ(0, test.Data()[3]);
  EXPECT_EQ('d', test.Data()[4]);
  EXPECT_EQ('e', test.Data()[5]);
  EXPECT_EQ('f', test.Data()[6]);
  EXPECT_EQ(0, test.Data()[7]);
  EXPECT_EQ('\n', test.Data()[8]);
}
