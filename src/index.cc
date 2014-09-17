// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "index.h"

#include "util.h"

const char* const kMagicHeader = "delve index v 1\n";
const char* const kMagicFooter = "\ndelve file end\n";

Index::Index(const string& filename) : mmap_(filename) {
  if (mmap_.Size() <
      2 * sizeof(uint32_t) + strlen(kMagicHeader) + strlen(kMagicFooter)) {
    Corrupt();
  }
  size_t n = mmap_.Size() - strlen(kMagicFooter) - 2 * sizeof(int);
  name_data_ = Uint32(n);
  name_index_ = Uint32(n + 4);
}

const char* Index::NameBytes(int index) {
  uint32_t offset = Uint32(name_index_ + sizeof(uint32_t) * index);
  return reinterpret_cast<const char*>(&mmap_.Data()[name_data_ + offset]);
}

void Index::Corrupt() {
  Fatal("index corrupt");
}

uint32_t Index::Uint32(size_t offset) {
  if (offset + sizeof(uint32_t) > mmap_.Size())
    Corrupt();
  return *(uint32_t*)&mmap_.Data()[offset];
}
