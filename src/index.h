// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DELVE_INDEX_H_
#define DELVE_INDEX_H_

#include "memory_mapped_file.h"

#include <stdint.h>

#include <string>
using namespace std;

// "delve index v 1\n"
// list of names
// name index
// footer
//
// The list of names is a sorted sequence of NUL terminated file names.
// They are 0-indexed. The name index is a sequence of 4 byte offsets listing
// the byte offset in the name list to where each name begins.
//
// The footer has the form:
// offset of name list [4]
// offset of name index [4]
// "\ndelve file end\n"
//
// All indices are little endian.

struct Index {
  explicit Index(const string& filename);

  const char* NameBytes(int index);

 private:
  void Corrupt();
  uint32_t Uint32(size_t offset);

  MemoryMappedFile mmap_;
  uint32_t name_data_;
  uint32_t name_index_;
};

#endif  // DELVE_INDEX_H_
