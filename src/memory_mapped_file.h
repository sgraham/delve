// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DELVE_MEMORY_MAPPED_FILE_H
#define DELVE_MEMORY_MAPPED_FILE_H

#include <assert.h>
#include <string>
#include <windows.h>
using namespace std;

class MemoryMappedFile {
public:
  MemoryMappedFile(const string& filename,
                   size_t initial_size = 1000000,
                   double grow_factor = 1.5);
  ~MemoryMappedFile();

  // Must be called before use. Returns true if the file was newly created and
  // so the caller should do first time setup.
  bool Initialize();

  void IncreaseFileSize();
  size_t Size() const { assert(file_ != INVALID_HANDLE_VALUE); return size_; }
  void* View() const { assert(file_ != INVALID_HANDLE_VALUE); return view_; }

private:
  void MapFile();
  void UnmapFile();

  HANDLE file_;
  HANDLE file_mapping_;
  string filename_;
  void* view_;
  size_t size_;
  size_t initial_size_;
  double grow_factor_;
};

#endif  // DELVE_MEMORY_MAPPED_FILE_H
