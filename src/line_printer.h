// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DELVE_LINE_PRINTER_H_
#define DELVE_LINE_PRINTER_H_

#include <stddef.h>
#include <string>
using namespace std;

/// Prints lines of text, possibly overprinting previously printed lines
/// if the terminal supports it.
struct LinePrinter {
  LinePrinter();

  bool is_smart_terminal() const { return smart_terminal_; }
  void set_smart_terminal(bool smart) { smart_terminal_ = smart; }

  enum LineType {
    FULL,
    ELIDE
  };
  /// Overprints the current line. If type is ELIDE, elides to_print to fit on
  /// one line.
  void Print(string to_print, LineType type);

  /// Prints a string on a new line, not overprinting previous output.
  void PrintOnNewLine(const string& to_print);

  /// Lock or unlock the console.  Any output sent to the LinePrinter while the
  /// console is locked will not be printed until it is unlocked.
  void SetConsoleLocked(bool locked);

 private:
  /// Whether we can do fancy terminal control codes.
  bool smart_terminal_;

  /// Whether the caret is at the beginning of a blank line.
  bool have_blank_line_;

  /// Whether console is locked.
  bool console_locked_;

  /// Buffered current line while console is locked.
  string line_buffer_;

  /// Buffered line type while console is locked.
  LineType line_type_;

  /// Buffered console output while console is locked.
  string output_buffer_;

#ifdef _WIN32
  void* console_;
#endif

  /// Print the given data to the console, or buffer it if it is locked.
  void PrintOrBuffer(const char *data, size_t size);
};

#endif  // DELVE_LINE_PRINTER_H_
