// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <vector>
#include <windows.h>
using namespace std;

struct FullWindowOutput {
  FullWindowOutput();
  ~FullWindowOutput();

  void Status(const string& status);

 private:
  void CaptureOriginalContentsAndClear();
  void RestoreOriginalContents();

#ifdef _WIN32
  void* console_;
  CHAR_INFO* original_contents_;
  int width_;
  int height_;
#endif
  bool smart_terminal_;
};
