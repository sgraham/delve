// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "full_window_output.h"
#include "util.h"

#include <conio.h>

struct FileListDatabase {
  struct FileReader {
    virtual ~FileReader() {}
    virtual bool ReadFile(const string &path, string *content, string *err) = 0;
  };

  explicit FileListDatabase(FileReader* file_reader)
      : file_reader_(file_reader) {}

  bool Load(const string& filename, string* err);

  size_t FileCount() const { return files_.size(); }

 private:
  FileReader* file_reader_;
  vector<string> files_;
};

bool FileListDatabase::Load(const string& filename, string* err) {
  string contents;
  string read_err;
  if (!file_reader_->ReadFile(filename, &contents, &read_err)) {
    *err = "loading '" + filename + "': " + read_err;
    return false;
  }

  files_.reserve(10000000);
  string cur;
  for (string::const_iterator i(contents.begin()); i != contents.end(); ++i) {
    if (*i == '\n') {
      files_.push_back(cur);
      cur.clear();
    }
  }
  if (!cur.empty())
    Fatal("expecting \n terminated db");
  return true;
}

struct RealFileReader : public FileListDatabase::FileReader {
  virtual bool ReadFile(const string &path, string *content, string *err) {
    return ::ReadFile(path, content, err) == 0;
  }
};

void BlockingInputLoop(void (*refresh_callback)(const string&, void*),
                       void* user_data) {
  HANDLE stdin_handle = ::GetStdHandle(STD_INPUT_HANDLE);
  if (stdin_handle == INVALID_HANDLE_VALUE)
    Fatal("GetStdHandle");
  DWORD old_mode;
  if (!::GetConsoleMode(stdin_handle, &old_mode))
    Fatal("GetConsoleMode");

  // Enable the window and mouse input events.

  DWORD input_mode = ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
  if (!::SetConsoleMode(stdin_handle, input_mode)) {
    SetConsoleMode(stdin_handle, old_mode);
    Fatal("SetConsoleMode");
  }

  string filter;

  for (;;) {
    DWORD num_read;
    INPUT_RECORD input_record[128];
    if (!ReadConsoleInput(stdin_handle,  // input buffer handle
                          input_record,  // buffer to read into
                          sizeof(input_record) /
                              sizeof(*input_record),  // size of read buffer
                          &num_read)) {               // number of records read
      SetConsoleMode(stdin_handle, old_mode);
      Fatal("ReadConsoleInput");
    }
    for (DWORD i = 0; i < num_read; i++) {
      switch (input_record[i].EventType) {
        case KEY_EVENT: {
          const KEY_EVENT_RECORD& ker = input_record[i].Event.KeyEvent;
          if (ker.wVirtualKeyCode == VK_ESCAPE) {
            goto done;
          } else if (ker.wVirtualKeyCode == VK_BACK && !filter.empty() &&
                     ker.bKeyDown) {
            filter = filter.substr(0, filter.size() - 1);
          } else if (isprint(ker.uChar.AsciiChar) && ker.bKeyDown) {
            filter += ker.uChar.AsciiChar;
          } else {
            //printf("%d\n", ker.wVirtualKeyCode);
          }
        } break;

        case MOUSE_EVENT:
        case WINDOW_BUFFER_SIZE_EVENT:
        case FOCUS_EVENT:
        case MENU_EVENT:
          // Ignore all of these.
          break;
      }

      refresh_callback(filter, user_data);
    }
  }

done:
  SetConsoleMode(stdin_handle, old_mode);
}

void Refresh(const string& filter, void* user_data) {
  FullWindowOutput* output = reinterpret_cast<FullWindowOutput*>(user_data);
  output->DisplayCurrentFilter(filter);
}

int main() {
  FullWindowOutput output;
  RealFileReader file_reader;
  FileListDatabase database(&file_reader);
  output.Status("Loading database...");

  string err;
  if (!database.Load("test.txt", &err)) {
    Fatal(err.c_str());
  }

  // type to filter
  // Ctrl-J/K to move in list
  // Ctrl-N = toggle { search file names, search contents }
  // Ctrl-R = toggle { plain substr, regex match }
  char buf[256];
  sprintf(buf,
          "Loaded %d files.",
          database.FileCount());
  output.Status(buf);

  BlockingInputLoop(&Refresh, reinterpret_cast<void*>(&output));

  return 0;
}
