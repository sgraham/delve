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

int main() {
  FullWindowOutput output;
  RealFileReader file_reader;
  FileListDatabase database(&file_reader);
  //output.Mode1("file names (Ctrl-N)");
  //output.Mode2("substring (Ctrl-R)");
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

  _getch();

  return 0;
}
