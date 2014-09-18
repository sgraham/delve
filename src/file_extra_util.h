// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DELVE_FILE_EXTRA_UTIL_H_
#define DELVE_FILE_EXTRA_UTIL_H_

#include <windows.h>

HANDLE OpenVolume(wchar_t drive_letter, bool async);
wchar_t GetCurrentVolume();

#endif  // DELVE_FILE_EXTRA_UTIL_H_
