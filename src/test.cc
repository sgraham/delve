// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdarg.h>
#include <stdio.h>

#include "test.h"
#include "line_printer.h"

#include <direct.h>

#ifdef _WIN32
#include <windows.h>
#pragma warning(disable : 4996)
#endif

static testing::Test* (*tests[10000])();
testing::Test* g_current_test;
static int ntests;
static LinePrinter printer;

namespace {

#ifdef _WIN32
#ifndef _mktemp_s
/// mingw has no mktemp.  Implement one with the same type as the one
/// found in the Windows API.
int _mktemp_s(char* templ) {
  char* ofs = strchr(templ, 'X');
  sprintf(ofs, "%d", rand() % 1000000);
  return 0;
}
#endif

/// Windows has no mkdtemp.  Implement it in terms of _mktemp_s.
char* mkdtemp(char* name_template) {
  int err = _mktemp_s(name_template);
  if (err < 0) {
    perror("_mktemp_s");
    return NULL;
  }

  err = _mkdir(name_template);
  if (err < 0) {
    perror("mkdir");
    return NULL;
  }

  return name_template;
}
#endif  // _WIN32

string GetSystemTempDir() {
#ifdef _WIN32
  char buf[1024];
  if (!GetTempPath(sizeof(buf), buf))
    return "";
  return buf;
#else
  const char* tempdir = getenv("TMPDIR");
  if (tempdir)
    return tempdir;
  return "/tmp";
#endif
}

string GetCurDir() {
  char buf[_MAX_PATH];
  _getcwd(buf, sizeof(buf));
  return buf;
}

}  // anonymous namespace

void RegisterTest(testing::Test* (*factory)()) {
  tests[ntests++] = factory;
}

string StringPrintf(const char* format, ...) {
  const int N = 1024;
  char buf[N];

  va_list ap;
  va_start(ap, format);
  vsnprintf(buf, N, format, ap);
  va_end(ap);

  return buf;
}

bool testing::Test::Check(bool condition, const char* file, int line,
                          const char* error) {
  if (!condition) {
    printer.PrintOnNewLine(
        StringPrintf("*** Failure in %s:%d\n%s\n", file, line, error));
    failed_ = true;
  }
  return condition;
}

void ScopedTempDir::CreateAndEnter(const string& name) {
  original_dir_ = GetCurDir();

  // First change into the system temp dir and save it for cleanup.
  start_dir_ = GetSystemTempDir();
  if (start_dir_.empty())
    Fatal("couldn't get system temp dir");
  if (chdir(start_dir_.c_str()) < 0)
    Fatal("chdir: %s", strerror(errno));

  // Create a temporary subdirectory of that.
  char name_template[1024];
  strcpy(name_template, name.c_str());
  strcat(name_template, "-XXXXXX");
  char* tempname = mkdtemp(name_template);
  if (!tempname)
    Fatal("mkdtemp: %s", strerror(errno));
  temp_dir_name_ = tempname;

  // chdir into the new temporary directory.
  if (chdir(temp_dir_name_.c_str()) < 0)
    Fatal("chdir: %s", strerror(errno));
}

void ScopedTempDir::Cleanup() {
  if (temp_dir_name_.empty())
    return;  // Something went wrong earlier.

  // Move out of the directory we're about to clobber.
  if (chdir(start_dir_.c_str()) < 0)
    Fatal("chdir: %s", strerror(errno));

#ifdef _WIN32
  string command = "rmdir /s /q " + temp_dir_name_;
#else
  string command = "rm -rf " + temp_dir_name_;
#endif
  if (system(command.c_str()) < 0)
    Fatal("system: %s", strerror(errno));

  temp_dir_name_.clear();

  if (chdir(original_dir_.c_str()) < 0)
    Fatal("chdir: %s", strerror(errno));
}

int main() {
  int tests_started = 0;
  int tests_failed = 0;

  for (int i = 0; i < ntests; i++) {
    ++tests_started;

    testing::Test* test = tests[i]();

    printer.Print(
        StringPrintf("[%d/%d] %s", tests_started, ntests, test->Name()),
        LinePrinter::ELIDE);

    test->SetUp();
    test->Run();
    test->TearDown();
    if (test->Failed())
      ++tests_failed;
    delete test;
  }

  printer.Print(tests_failed == 0
                    ? StringPrintf("all %d tests ok", ntests)
                    : StringPrintf("%d/%d tests failed", tests_failed, ntests),
                LinePrinter::ELIDE);
  return tests_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
