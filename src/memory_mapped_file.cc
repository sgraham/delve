// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "memory_mapped_file.h"
#include "util.h"

MemoryMappedFile::MemoryMappedFile(const string& filename)
    : file_(INVALID_HANDLE_VALUE),
      file_mapping_(INVALID_HANDLE_VALUE),
      view_(nullptr),
      size_(0) {
  file_ = ::CreateFileA(filename.c_str(),
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
  if (file_ == INVALID_HANDLE_VALUE)
    Win32Fatal("CreateFile");
  size_ = ::GetFileSize(file_, NULL);

  file_mapping_ = ::CreateFileMapping(file_, NULL, PAGE_READWRITE, 0, 0, NULL);
  if (file_mapping_ == INVALID_HANDLE_VALUE)
    Win32Fatal("CreateFileMapping");
  view_ =
      ::MapViewOfFile(file_mapping_, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
  if (!view_)
    Win32Fatal("MapViewOfFile");
}

MemoryMappedFile::~MemoryMappedFile() {
  if (view_)
    if (!::UnmapViewOfFile(view_))
      Win32Fatal("UnmapViewOfFile");
  view_ = nullptr;
  if (file_mapping_ != INVALID_HANDLE_VALUE)
    if (!::CloseHandle(file_mapping_))
      Win32Fatal("CloseHandle file_mapping_");
  file_mapping_ = INVALID_HANDLE_VALUE;
  if (!::CloseHandle(file_))
    Fatal("CloseHandle: file_");
}
