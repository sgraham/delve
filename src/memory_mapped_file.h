// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DELVE_MEMORY_MAPPED_FILE_H
#define DELVE_MEMORY_MAPPED_FILE_H

#include <string>
#include <windows.h>
using namespace std;

class MemoryMappedFile {
public:
  MemoryMappedFile(const string& filename);
  ~MemoryMappedFile();

  size_t Size() const { return size_; }
  const unsigned char* Data() const {
    return reinterpret_cast<const unsigned char*>(view_);
  }

private:
  void MapFile();
  void UnmapFile();

  HANDLE file_;
  HANDLE file_mapping_;
  void* view_;
  size_t size_;
};

#endif  // DELVE_MEMORY_MAPPED_FILE_H
