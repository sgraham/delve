// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "line_printer.h"
#include "util.h"

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
  LinePrinter line_printer;
  RealFileReader file_reader;
  FileListDatabase database(&file_reader);
  line_printer.Print("Loading database...", LinePrinter::ELIDE);

  string err;
  if (!database.Load("test.txt", &err)) {
    Fatal(err.c_str());
  }

  char buf[256];
  sprintf(buf, "Loaded %d files.", database.FileCount());
  line_printer.Print(buf, LinePrinter::ELIDE);



  return 0;
}
