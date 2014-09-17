// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "memory_mapped_file.h"
#include "util.h"

MemoryMappedFile::MemoryMappedFile(const string& filename,
                                   size_t initial_size,
                                   double grow_factor) :
    file_(INVALID_HANDLE_VALUE),
    file_mapping_(INVALID_HANDLE_VALUE),
    filename_(filename),
    view_(NULL),
    size_(0),
    initial_size_(initial_size),
    grow_factor_(grow_factor) {
}

MemoryMappedFile::~MemoryMappedFile() {
  UnmapFile();
  if (!CloseHandle(file_))
    Fatal("CloseHandle: file_");
}

bool MemoryMappedFile::Initialize() {
  bool should_initialize = false;
  file_ = CreateFileA(filename_.c_str(), GENERIC_READ | GENERIC_WRITE,
                      FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                      CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
  if (file_ == INVALID_HANDLE_VALUE)
    file_ = CreateFileA(filename_.c_str(), GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (file_ == INVALID_HANDLE_VALUE)
    Fatal("CreateFile (%d)", GetLastError());
  size_ = GetFileSize(file_, NULL);
  if (size_ == 0) {
    should_initialize = true;
    IncreaseFileSize();
  }

  MapFile();

  return should_initialize;
}

void MemoryMappedFile::IncreaseFileSize() {
  UnmapFile();
  size_t target_size = size_ == 0 ?
      initial_size_ :
      static_cast<size_t>(size_ * grow_factor_);
  if (SetFilePointer(file_, static_cast<LONG>(target_size), NULL, FILE_BEGIN) ==
      INVALID_SET_FILE_POINTER)
    Fatal("SetFilePointer (%d)", GetLastError());
  if (!SetEndOfFile(file_))
    Fatal("SetEndOfFile (%d)", GetLastError());
  size_ = GetFileSize(file_, NULL);
  if (size_ != target_size)
    Fatal("File resize failed");
  MapFile();
}

void MemoryMappedFile::MapFile() {
  if (file_mapping_ != INVALID_HANDLE_VALUE)
    return;
  file_mapping_ = CreateFileMapping(file_, NULL, PAGE_READWRITE, 0, 0, NULL);
  if (file_mapping_ == INVALID_HANDLE_VALUE)
    Fatal("CreateFileMapping (%d)", GetLastError());
  view_ = MapViewOfFile(file_mapping_, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
  if (!view_)
    Fatal("MapViewOfFile (%d)", GetLastError());
}

void MemoryMappedFile::UnmapFile() {
  if (view_)
    if (!UnmapViewOfFile(view_))
      Fatal("UnmapViewOfFile (%d)", GetLastError());
  view_ = 0;
  if (file_mapping_ != INVALID_HANDLE_VALUE)
    if (!CloseHandle(file_mapping_))
      Fatal("CloseHandle: file_mapping_ (%d)", GetLastError());
  file_mapping_ = INVALID_HANDLE_VALUE;
}
