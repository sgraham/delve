// // Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DELVE_TEST_H_
#define DELVE_TEST_H_

#include "util.h"

namespace testing {
class Test {
  bool failed_;
  int assertion_failures_;
  Test* next_;
 public:
  Test() : failed_(false), assertion_failures_(0) {}
  virtual ~Test() {}
  virtual void SetUp() {}
  virtual void TearDown() {}
  virtual void Run() = 0;
  virtual const char* Name() const = 0;

  bool Failed() const { return failed_; }
  int AssertionFailures() const { return assertion_failures_; }
  void AddAssertionFailure() { assertion_failures_++; }
  bool Check(bool condition, const char* file, int line, const char* error);
};
}

void RegisterTest(testing::Test* (*)());

extern testing::Test* g_current_test;
#define TEST_F_(x, y, name)                                           \
  struct y : public x {                                               \
    static testing::Test* Create() { return g_current_test = new y; } \
    virtual void Run();                                               \
    virtual const char* Name() const { return name; }                 \
  };                                                                  \
  struct Register##y {                                                \
    Register##y() { RegisterTest(y::Create); }                        \
  };                                                                  \
  Register##y g_register_##y;                                         \
  void y::Run()

#define TEST_F(x, y) TEST_F_(x, x##y, #x "." #y)
#define TEST(x, y) TEST_F_(testing::Test, x##y, #x "." #y)

#define EXPECT_EQ(a, b) \
  g_current_test->Check(a == b, __FILE__, __LINE__, #a " == " #b)
#define EXPECT_NE(a, b) \
  g_current_test->Check(a != b, __FILE__, __LINE__, #a " != " #b)
#define EXPECT_GT(a, b) \
  g_current_test->Check(a > b, __FILE__, __LINE__, #a " > " #b)
#define EXPECT_LT(a, b) \
  g_current_test->Check(a < b, __FILE__, __LINE__, #a " < " #b)
#define EXPECT_GE(a, b) \
  g_current_test->Check(a >= b, __FILE__, __LINE__, #a " >= " #b)
#define EXPECT_LE(a, b) \
  g_current_test->Check(a <= b, __FILE__, __LINE__, #a " <= " #b)
#define EXPECT_TRUE(a) \
  g_current_test->Check(static_cast<bool>(a), __FILE__, __LINE__, #a)
#define EXPECT_FALSE(a) \
  g_current_test->Check(!static_cast<bool>(a), __FILE__, __LINE__, #a)

#define ASSERT_EQ(a, b) \
  if (!EXPECT_EQ(a, b)) { g_current_test->AddAssertionFailure(); return; }
#define ASSERT_NE(a, b) \
  if (!EXPECT_NE(a, b)) { g_current_test->AddAssertionFailure(); return; }
#define ASSERT_GT(a, b) \
  if (!EXPECT_GT(a, b)) { g_current_test->AddAssertionFailure(); return; }
#define ASSERT_LT(a, b) \
  if (!EXPECT_LT(a, b)) { g_current_test->AddAssertionFailure(); return; }
#define ASSERT_GE(a, b) \
  if (!EXPECT_GE(a, b)) { g_current_test->AddAssertionFailure(); return; }
#define ASSERT_LE(a, b) \
  if (!EXPECT_LE(a, b)) { g_current_test->AddAssertionFailure(); return; }
#define ASSERT_TRUE(a)  \
  if (!EXPECT_TRUE(a))  { g_current_test->AddAssertionFailure(); return; }
#define ASSERT_FALSE(a) \
  if (!EXPECT_FALSE(a)) { g_current_test->AddAssertionFailure(); return; }
#define ASSERT_NO_FATAL_FAILURE(a)                  \
  {                                                 \
    int f = g_current_test->AssertionFailures();    \
    a;                                              \
    if (f != g_current_test->AssertionFailures()) { \
      g_current_test->AddAssertionFailure();        \
      return;                                       \
    }                                               \
  }


struct ScopedTempDir {
  /// Create a temporary directory and chdir into it.
  void CreateAndEnter(const string& name);

  /// Clean up the temporary directory.
  void Cleanup();

  /// The temp directory containing our dir.
  string start_dir_;
  /// The subdirectory name for our dir, or empty if it hasn't been set up.
  string temp_dir_name_;
};

#endif  // DELVE_TEST_H_
