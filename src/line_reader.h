// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DELVE_LINE_READER_H_
#define DELVE_LINE_READER_H_

class LineReader {
 public:
  explicit LineReader(FILE* file)
    : file_(file), buf_end_(buf_), line_start_(buf_), line_end_(NULL) {
      memset(buf_, 0, sizeof(buf_));
  }

  // Reads a \n-terminated line from the file passed to the constructor.
  // On return, *line_start points to the beginning of the next line, and
  // *line_end points to the \n at the end of the line. If no newline is seen
  // in a fixed buffer size, *line_end is set to NULL. Returns false on EOF.
  bool ReadLine(wchar_t** line_start, wchar_t** line_end) {
    if (line_start_ >= buf_end_ || !line_end_) {
      // Buffer empty, refill.
      size_t size_read = fread(buf_, 1, sizeof(buf_), file_);
      if (!size_read)
        return false;
      line_start_ = buf_;
      buf_end_ = buf_ + size_read;
    } else {
      // Advance to next line in buffer.
      line_start_ = line_end_ + 1;
    }

    line_end_ = (wchar_t*)wmemchr(line_start_, '\n', buf_end_ - line_start_);
    if (!line_end_) {
      // No newline. Move rest of data to start of buffer, fill rest.
      size_t already_consumed = line_start_ - buf_;
      size_t size_rest = (buf_end_ - buf_) - already_consumed;
      memmove(buf_, line_start_, size_rest);

      size_t read = fread(buf_ + size_rest, 1, sizeof(buf_) - size_rest, file_);
      buf_end_ = buf_ + size_rest + read;
      line_start_ = buf_;
      line_end_ = (wchar_t*)wmemchr(line_start_, '\n', buf_end_ - line_start_);
    }

    *line_start = line_start_;
    *line_end = line_end_;
    return true;
  }

 private:
  FILE* file_;
  wchar_t buf_[256 << 10];
  wchar_t* buf_end_;  // Points one past the last valid byte in |buf_|.

  wchar_t* line_start_;
  // Points at the next \n in buf_ after line_start, or NULL.
  wchar_t* line_end_;
};

#endif  // DELVE_LINE_READER_H_
