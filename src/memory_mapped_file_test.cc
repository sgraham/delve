// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "memory_mapped_file.h"

#include "test.h"

TEST(MemoryMappedFileTest, Empty) {
  ScopedTempDir temp;
  temp.CreateAndEnter("Empty");

  {
    MemoryMappedFile stuff("stuff", 10, 2);
    EXPECT_EQ(true, stuff.Initialize());
    EXPECT_EQ(10, stuff.Size());
  }

  temp.Cleanup();
}

TEST(MemoryMappedFileTest, Grow) {
  ScopedTempDir temp;
  temp.CreateAndEnter("Grow");

  {
    MemoryMappedFile stuff("stuff", 10, 2);
    EXPECT_EQ(true, stuff.Initialize());
    memcpy(stuff.View(), "HEADER", 6);
    stuff.IncreaseFileSize();
    stuff.IncreaseFileSize();
    EXPECT_EQ(40, stuff.Size());
    EXPECT_EQ(0, memcmp(stuff.View(), "HEADER", 6));
  }

  temp.Cleanup();
}
