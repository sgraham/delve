// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdarg.h>
#include <stdio.h>

#include "test.h"
#include "line_printer.h"

static testing::Test* (*tests[10000])();
testing::Test* g_current_test;
static int ntests;
static LinePrinter printer;

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

int main(int argc, char **argv) {
  int tests_started = 0;

  bool passed = true;
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
      passed = false;
    delete test;
  }

  printer.PrintOnNewLine(passed ? "passed\n" : "failed\n");
  return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
