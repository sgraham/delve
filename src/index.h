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
//
// Incremental updates:
// - Keep N-1 shards on disk, and Nth update one in memory in plain memory
// storage suitable for updates.
// - Removal/renames can be semi-ignored as the index will name a file that's
// gone. If it hits a the trigram query, the final grep can simply fail
// quietly.
// - File additions build index in memory. Again, conservative is OK. We need
// to add the trigrams for the new file right away, but we don't need to worry
// about fully superceeding previous versions.
// - When the in-memory exceeds some threshold, it's written to disk and then
// merged wth the other shards. The merge should tidy up the deletions, and
// needs to be careful to make sure the newest version from the in-memory
// version is the version that's kept. Timestamps?

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
