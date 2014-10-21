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
  void DisplayCurrentFilter(const string& status);
  int VisibleOutputLines() const;
  void DisplayResults(const vector<string>& results, int highlight);

 private:
  void CaptureOriginalContentsAndClear();
  void RestoreOriginalContents();

  // y_offset >= 0 from top, < 0 from bottom
  // |prefix| never truncated, |rest| elided in middle
  // if |reverse_video|, black on white instead of white on black.
  void FillLine(int y_offset,
                const string& prefix,
                const string& rest,
                bool reverse_video);

#ifdef _WIN32
  void* console_;
  CHAR_INFO* original_contents_;
  int width_;
  int height_;
#endif
  bool smart_terminal_;
};
