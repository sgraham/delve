// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "full_window_output.h"

#include "util.h"

#ifdef _WIN32
#include <windows.h>
#else
#error
#endif

FullWindowOutput::FullWindowOutput()
    : original_contents_(NULL), width_(-1), height_(-1) {
#ifndef _WIN32
  const char* term = getenv("TERM");
  smart_terminal_ = isatty(1) && term && string(term) != "dumb";
#else
  console_ = GetStdHandle(STD_OUTPUT_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  smart_terminal_ = GetConsoleScreenBufferInfo(console_, &csbi);
#endif

  CaptureOriginalContentsAndClear();
}

FullWindowOutput::~FullWindowOutput() {
  RestoreOriginalContents();
  delete original_contents_;
}

void FullWindowOutput::FillLine(int y_offset,
                                const string& prefix,
                                const string& rest,
                                bool reverse_video) {
  string to_print;
#ifdef _WIN32
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo(console_, &csbi);
  // Don't use the full width or console will move to next line.
  size_t width = static_cast<size_t>(csbi.dwSize.X - 1 - prefix.size());
  to_print = prefix + ElideMiddle(rest, width);
  GetConsoleScreenBufferInfo(console_, &csbi);
  COORD buf_size = {csbi.dwSize.X, 1};
  COORD zero_zero = {0, 0};
  SHORT y_coord = static_cast<SHORT>(
      y_offset >= 0 ? y_offset : csbi.srWindow.Bottom + y_offset + 1);
  SMALL_RECT target = {0,                                         y_coord,
                       static_cast<SHORT>(0 + csbi.dwSize.X - 1), y_coord};
  CHAR_INFO* char_data = new CHAR_INFO[csbi.dwSize.X];
  memset(char_data, 0, sizeof(CHAR_INFO) * csbi.dwSize.X);
  for (int i = 0; i < csbi.dwSize.X; ++i) {
    char_data[i].Char.AsciiChar = ' ';
    // Black on white.
    char_data[i].Attributes =
        reverse_video ? (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE)
                      : (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
  }
  for (size_t i = 0; i < to_print.size(); ++i)
    char_data[i].Char.AsciiChar = to_print[i];
  WriteConsoleOutput(console_, char_data, buf_size, zero_zero, &target);
  csbi.dwCursorPosition.X = static_cast<SHORT>(to_print.size());
  csbi.dwCursorPosition.Y = y_coord;
  SetConsoleCursorPosition(console_, csbi.dwCursorPosition);
  delete[] char_data;
#else
  // Limit output to width of the terminal if provided so we don't cause
  // line-wrapping.
  winsize size;
  if ((ioctl(0, TIOCGWINSZ, &size) == 0) && size.ws_col) {
    to_print = ElideMiddle(status, size.ws_col);
  }
  printf("%s", to_print.c_str());
  printf("\x1B[K");  // Clear to end of line.
  fflush(stdout);
#endif
}

void FullWindowOutput::Status(const string& status) {
  FillLine(
      -2, "" /*"[file names : Ctrl-N] [substring : Ctrl-R] "*/, status, true);
}

void FullWindowOutput::DisplayCurrentFilter(const string& filter) {
  FillLine(-1, "", filter, false);
}

void FullWindowOutput::CaptureOriginalContentsAndClear() {
  if (!smart_terminal_)
    Fatal("not a smart terminal");

#ifndef _WIN32
#error
#else
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo(console_, &csbi);
  width_ = csbi.srWindow.Right - csbi.srWindow.Left + 1;
  height_ = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
  COORD zero_zero = {0, 0};
  COORD window_size = {static_cast<SHORT>(width_), static_cast<SHORT>(height_)};
  COORD window_left_top = {csbi.srWindow.Left, csbi.srWindow.Top};
  original_contents_ = new CHAR_INFO[width_ * height_];
  if (!ReadConsoleOutput(console_,
                         original_contents_,
                         window_size,
                         zero_zero,
                         &csbi.srWindow)) {
    Win32Fatal("ReadConsoleOutput");
  }

  DWORD written;
  if (!FillConsoleOutputCharacter(
           console_, ' ', width_ * height_, window_left_top, &written)) {
    Win32Fatal("FillConsoleOutputCharacter");
  }
  if (!FillConsoleOutputAttribute(
           console_,
           FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
           width_ * height_,
           window_left_top,
           &written)) {
    Win32Fatal("FillConsoleOutputAttribute");
  }
#endif
}

void FullWindowOutput::RestoreOriginalContents() {
  if (!smart_terminal_)
    Fatal("not a smart terminal");

#ifndef _WIN32
#error
#else
#endif

  // TODO(scottmg): Resize during operation would crash; saving is weird too
  // though.
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo(console_, &csbi);
  COORD zero_zero = {0, 0};
  COORD window_size = {static_cast<SHORT>(width_), static_cast<SHORT>(height_)};
  if (!WriteConsoleOutput(console_,
                          original_contents_,
                          window_size,
                          zero_zero,
                          &csbi.srWindow)) {
    Win32Fatal("WriteConsoleOutput");
  }
}
