// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util.h"

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <share.h>
#endif

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef _WIN32
#include <unistd.h>
#include <sys/time.h>
#endif

#include <vector>

#if defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/sysctl.h>
#elif defined(__SVR4) && defined(__sun)
#include <unistd.h>
#include <sys/loadavg.h>
#elif defined(linux) || defined(__GLIBC__)
#include <sys/sysinfo.h>
#endif

void Fatal(const char* msg, ...) {
  va_list ap;
  fprintf(stderr, "\ndelve: fatal: ");
  va_start(ap, msg);
  vfprintf(stderr, msg, ap);
  va_end(ap);
  fprintf(stderr, "\n");
#ifdef _WIN32
  // On Windows, some tools may inject extra threads.
  // exit() may block on locks held by those threads, so forcibly exit.
  fflush(stderr);
  fflush(stdout);
  ExitProcess(1);
#else
  exit(1);
#endif
}

void Warning(const char* msg, ...) {
  va_list ap;
  fprintf(stderr, "\ndelve: warning: ");
  va_start(ap, msg);
  vfprintf(stderr, msg, ap);
  va_end(ap);
  fprintf(stderr, "\n");
}

void Error(const char* msg, ...) {
  va_list ap;
  fprintf(stderr, "\ndelve: error: ");
  va_start(ap, msg);
  vfprintf(stderr, msg, ap);
  va_end(ap);
  fprintf(stderr, "\n");
}

bool CanonicalizePath(string* path, string* err) {
  size_t len = path->size();
  char* str = 0;
  if (len > 0)
    str = &(*path)[0];
  if (!CanonicalizePath(str, &len, err))
    return false;
  path->resize(len);
  return true;
}

bool CanonicalizePath(char* path, size_t* len, string* err) {
  // WARNING: this function is performance-critical; please benchmark
  // any changes you make to it.
  if (*len == 0) {
    *err = "empty path";
    return false;
  }

  const int kMaxPathComponents = 30;
  char* components[kMaxPathComponents];
  int component_count = 0;

  char* start = path;
  char* dst = start;
  const char* src = start;
  const char* end = start + *len;

  if (*src == '/') {
#ifdef _WIN32
    // network path starts with //
    if (*len > 1 && *(src + 1) == '/') {
      src += 2;
      dst += 2;
    } else {
      ++src;
      ++dst;
    }
#else
    ++src;
    ++dst;
#endif
  }

  while (src < end) {
    if (*src == '.') {
      if (src + 1 == end || src[1] == '/') {
        // '.' component; eliminate.
        src += 2;
        continue;
      } else if (src[1] == '.' && (src + 2 == end || src[2] == '/')) {
        // '..' component.  Back up if possible.
        if (component_count > 0) {
          dst = components[component_count - 1];
          src += 3;
          --component_count;
        } else {
          *dst++ = *src++;
          *dst++ = *src++;
          *dst++ = *src++;
        }
        continue;
      }
    }

    if (*src == '/') {
      src++;
      continue;
    }

    if (component_count == kMaxPathComponents)
      Fatal("path has too many components : %s", path);
    components[component_count] = dst;
    ++component_count;

    while (*src != '/' && src != end)
      *dst++ = *src++;
    *dst++ = *src++;  // Copy '/' or final \0 character as well.
  }

  if (dst == start) {
    *err = "path canonicalizes to the empty path";
    return false;
  }

  *len = dst - start - 1;
  return true;
}

static inline bool IsKnownShellSafeCharacter(char ch) {
  if ('A' <= ch && ch <= 'Z') return true;
  if ('a' <= ch && ch <= 'z') return true;
  if ('0' <= ch && ch <= '9') return true;

  switch (ch) {
    case '_':
    case '+':
    case '-':
    case '.':
    case '/':
      return true;
    default:
      return false;
  }
}

static inline bool IsKnownWin32SafeCharacter(char ch) {
  switch (ch) {
    case ' ':
    case '"':
      return false;
    default:
      return true;
  }
}

static inline bool StringNeedsShellEscaping(const string& input) {
  for (size_t i = 0; i < input.size(); ++i) {
    if (!IsKnownShellSafeCharacter(input[i])) return true;
  }
  return false;
}

static inline bool StringNeedsWin32Escaping(const string& input) {
  for (size_t i = 0; i < input.size(); ++i) {
    if (!IsKnownWin32SafeCharacter(input[i])) return true;
  }
  return false;
}

void GetShellEscapedString(const string& input, string* result) {
  assert(result);

  if (!StringNeedsShellEscaping(input)) {
    result->append(input);
    return;
  }

  const char kQuote = '\'';
  const char kEscapeSequence[] = "'\\'";

  result->push_back(kQuote);

  string::const_iterator span_begin = input.begin();
  for (string::const_iterator it = input.begin(), end = input.end(); it != end;
       ++it) {
    if (*it == kQuote) {
      result->append(span_begin, it);
      result->append(kEscapeSequence);
      span_begin = it;
    }
  }
  result->append(span_begin, input.end());
  result->push_back(kQuote);
}


void GetWin32EscapedString(const string& input, string* result) {
  assert(result);
  if (!StringNeedsWin32Escaping(input)) {
    result->append(input);
    return;
  }

  const char kQuote = '"';
  const char kBackslash = '\\';

  result->push_back(kQuote);
  size_t consecutive_backslash_count = 0;
  string::const_iterator span_begin = input.begin();
  for (string::const_iterator it = input.begin(), end = input.end(); it != end;
       ++it) {
    switch (*it) {
      case kBackslash:
        ++consecutive_backslash_count;
        break;
      case kQuote:
        result->append(span_begin, it);
        result->append(consecutive_backslash_count + 1, kBackslash);
        span_begin = it;
        consecutive_backslash_count = 0;
        break;
      default:
        consecutive_backslash_count = 0;
        break;
    }
  }
  result->append(span_begin, input.end());
  result->append(consecutive_backslash_count, kBackslash);
  result->push_back(kQuote);
}

int ReadFile(const string& path, string* contents, string* err) {
  FILE* f = fopen(path.c_str(), "r");
  if (!f) {
    err->assign(strerror(errno));
    return -errno;
  }

  char buf[64 << 10];
  size_t len;
  while ((len = fread(buf, 1, sizeof(buf), f)) > 0) {
    contents->append(buf, len);
  }
  if (ferror(f)) {
    err->assign(strerror(errno));  // XXX errno?
    contents->clear();
    fclose(f);
    return -errno;
  }
  fclose(f);
  return 0;
}

void SetCloseOnExec(int fd) {
#ifndef _WIN32
  int flags = fcntl(fd, F_GETFD);
  if (flags < 0) {
    perror("fcntl(F_GETFD)");
  } else {
    if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) < 0)
      perror("fcntl(F_SETFD)");
  }
#else
  HANDLE hd = (HANDLE) _get_osfhandle(fd);
  if (! SetHandleInformation(hd, HANDLE_FLAG_INHERIT, 0)) {
    fprintf(stderr, "SetHandleInformation(): %s", GetLastErrorString().c_str());
  }
#endif  // ! _WIN32
}

#ifdef _WIN32
string GetLastErrorString() {
  DWORD err = GetLastError();

  char* msg_buf;
  FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        err,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (char*)&msg_buf,
        0,
        NULL);
  string msg = msg_buf;
  LocalFree(msg_buf);
  return msg;
}

void Win32Fatal(const char* function) {
  Fatal("%s: %s", function, GetLastErrorString().c_str());
}
#endif

static bool islatinalpha(int c) {
  // isalpha() is locale-dependent.
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

string StripAnsiEscapeCodes(const string& in) {
  string stripped;
  stripped.reserve(in.size());

  for (size_t i = 0; i < in.size(); ++i) {
    if (in[i] != '\33') {
      // Not an escape code.
      stripped.push_back(in[i]);
      continue;
    }

    // Only strip CSIs for now.
    if (i + 1 >= in.size()) break;
    if (in[i + 1] != '[') continue;  // Not a CSI.
    i += 2;

    // Skip everything up to and including the next [a-zA-Z].
    while (i < in.size() && !islatinalpha(in[i]))
      ++i;
  }
  return stripped;
}

int GetProcessorCount() {
#ifdef _WIN32
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  return info.dwNumberOfProcessors;
#else
  return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

string ElideMiddle(const string& str, size_t width) {
  const int kMargin = 3;  // Space for "...".
  string result = str;
  if (result.size() + kMargin > width) {
    size_t elide_size = (width - kMargin) / 2;
    result = result.substr(0, elide_size)
      + "..."
      + result.substr(result.size() - elide_size, elide_size);
  }
  return result;
}

bool Truncate(const string& path, size_t size, string* err) {
#ifdef _WIN32
  int fh = _sopen(path.c_str(), _O_RDWR | _O_CREAT, _SH_DENYNO,
                  _S_IREAD | _S_IWRITE);
  int success = _chsize_s(fh, size);
  _close(fh);
#else
  int success = truncate(path.c_str(), size);
#endif
  // Both truncate() and _chsize() return 0 on success and set errno and return
  // -1 on failure.
  if (success < 0) {
    *err = strerror(errno);
    return false;
  }
  return true;
}
