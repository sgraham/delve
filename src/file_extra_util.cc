// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "file_extra_util.h"
#include <string>
using namespace std;

#include "util.h"

HANDLE OpenVolume(wchar_t drive_letter, bool async) {
  drive_letter = static_cast<wchar_t>(toupper(drive_letter));
  wstring volume_path = wstring(L"\\\\.\\") + drive_letter + L":";
  HANDLE cj = ::CreateFileW(
      volume_path.c_str(), GENERIC_WRITE | GENERIC_READ,
      FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
      (async ? FILE_FLAG_OVERLAPPED : 0), NULL);
  return cj;
}

wchar_t GetCurrentVolume() {
  wchar_t cur_drive[32];
  if (!::GetVolumePathNameW(L".", cur_drive, sizeof(cur_drive)))
    Win32Fatal("GetVolumePathName");
  return static_cast<wchar_t>(toupper(cur_drive[0]));
}
