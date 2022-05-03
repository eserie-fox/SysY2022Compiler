#pragma once

#define IDENTICAL(val) val

#define NONCOPYABLE(T)   \
  T(const T &) = delete; \
  T &operator=(const T &) = delete;


  