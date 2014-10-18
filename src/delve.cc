// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "full_window_output.h"
#include "util.h"
#include "re2/re2.h"

#include <conio.h>

struct FileListDatabase {
  struct FileReader {
    virtual ~FileReader() {}
    virtual bool ReadFile(const string &path, string *content, string *err) = 0;
  };

  explicit FileListDatabase(FileReader* file_reader)
      : file_reader_(file_reader) {}

  bool Load(const string& filename, string* err);

  const vector<string>& Files() const { return files_; }

 private:
  FileReader* file_reader_;
  vector<string> files_;

  DISALLOW_COPY_AND_ASSIGN(FileListDatabase);
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
    } else
      cur += *i;
  }
  if (!cur.empty())
    Fatal("expecting \n terminated db");
  return true;
}

struct RealFileReader : public FileListDatabase::FileReader {
  RealFileReader() {}
  virtual bool ReadFile(const string &path, string *content, string *err) {
    return ::ReadFile(path, content, err) == 0;
  }

  DISALLOW_COPY_AND_ASSIGN(RealFileReader);
};

struct SearchResult {
  string filename;
  int line;
  string contents;
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
    for (unsigned int i = 0; i < num_read; i++) {
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

void RefreshThunk(const string& filter, void* user_data);

class Entry {
 public:
  Entry() : database_(&file_reader_) {}

  void Run() {
    output_.Status("Loading database...");
    string err;
    if (!database_.Load("test.txt", &err))
      Fatal(err.c_str());
    char buf[256];  // TODO
    sprintf(buf, "Loaded %d files.", database_.Files().size());
    output_.Status(buf);

    BlockingInputLoop(&RefreshThunk, reinterpret_cast<void*>(this));
  }

  void Refresh(const string& filter) {
    output_.DisplayCurrentFilter(filter);
    vector<SearchResult> results =
        BruteForceFiles(filter, output_.VisibleOutputLines());
    vector<string> present;
    for (const auto& result : results) {
      char buf[1024];  // TODO
      sprintf(buf,
              "%s:%d:%s",
              result.filename.c_str(),
              result.line,
              result.contents.c_str());
      present.push_back(buf);
    }
    output_.DisplayResults(present);
  }

 private:
  vector<SearchResult> BruteForceFiles(const string& filter, int limit) {
    vector<SearchResult> ret;
    RE2 pattern(filter);
    for (const auto& file : database_.Files()) {
      int line = 1;
      string contents;
      string err;
      if (!file_reader_.ReadFile(file, &contents, &err))
        Fatal(err.c_str());
      SearchResult result;
      string::const_iterator p = contents.begin();
      string::const_iterator end = contents.begin();
      for (;;) {
        string::const_iterator nl = find(p, end, '\n');
        re2::StringPiece piece(&*p, static_cast<int>(nl - p));
        if (RE2::PartialMatch(piece, pattern)) {
          SearchResult result;
          result.filename = file;
          result.line = line;
          result.contents = piece.ToString();
          ret.push_back(result);
          if (static_cast<int>(ret.size()) >= limit)
            return ret;
        }
        if (nl == end)
          break;
        ++line;
        p = nl + 1;
      }
    }
    return ret;
  }

  FullWindowOutput output_;
  RealFileReader file_reader_;
  FileListDatabase database_;

  DISALLOW_COPY_AND_ASSIGN(Entry);
};

void RefreshThunk(const string& filter, void* user_data) {
  Entry* entry = reinterpret_cast<Entry*>(user_data);
  entry->Refresh(filter);
}

int main() {
  Entry entry;
  entry.Run();
  return 0;
}
