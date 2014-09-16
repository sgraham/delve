// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DELVE_UTIL_H_
#define DELVE_UTIL_H_

#include <stdint.h>

#include <string>
#include <vector>
using namespace std;

#ifdef _MSC_VER
#define NORETURN __declspec(noreturn)
#else
#define NORETURN __attribute__((noreturn))
#endif

/// Log a fatal message and exit.
NORETURN void Fatal(const char* msg, ...);

/// Log a warning message.
void Warning(const char* msg, ...);

/// Log an error message.
void Error(const char* msg, ...);

/// Canonicalize a path like "foo/../bar.h" into just "bar.h".
bool CanonicalizePath(string* path, string* err);

bool CanonicalizePath(char* path, size_t* len, string* err);

/// Appends |input| to |*result|, escaping according to the whims of either
/// Bash, or Win32's CommandLineToArgvW().
/// Appends the string directly to |result| without modification if we can
/// determine that it contains no problematic characters.
void GetShellEscapedString(const string& input, string* result);
void GetWin32EscapedString(const string& input, string* result);

/// Read a file to a string (in text mode: with CRLF conversion
/// on Windows).
/// Returns -errno and fills in \a err on error.
int ReadFile(const string& path, string* contents, string* err);

/// Mark a file descriptor to not be inherited on exec()s.
void SetCloseOnExec(int fd);

/// Removes all Ansi escape codes (http://www.termsys.demon.co.uk/vtansi.htm).
string StripAnsiEscapeCodes(const string& in);

/// @return the number of processors on the machine.  Useful for an initial
/// guess for how many jobs to run in parallel.  @return 0 on error.
int GetProcessorCount();

/// @return the load average of the machine. A negative value is returned
/// on error.
double GetLoadAverage();

/// Elide the given string @a str with '...' in the middle if the length
/// exceeds @a width.
string ElideMiddle(const string& str, size_t width);

/// Truncates a file to the given size.
bool Truncate(const string& path, size_t size, string* err);

#ifdef _MSC_VER
#define snprintf _snprintf
#define fileno _fileno
#define unlink _unlink
#define chdir _chdir
#define strtoull _strtoui64
#define getcwd _getcwd
#define PATH_MAX _MAX_PATH
#endif

#ifdef _WIN32
/// Convert the value returned by GetLastError() into a string.
string GetLastErrorString();

/// Calls Fatal() with a function name and GetLastErrorString.
NORETURN void Win32Fatal(const char* function);
#endif

#endif  // DELVE_UTIL_H_
