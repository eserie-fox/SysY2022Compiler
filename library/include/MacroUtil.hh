#pragma once

#define IDENTICAL(val) val

#define NONCOPYABLE(T)   \
  T(const T &) = delete; \
  T &operator=(const T &) = delete;

#define ISSET_UINT(uint_val, pos) (!!((uint_val) & (1U << (pos))))

#define SET_UINT(uint_val, pos) ((uint_val) |= (1U << (pos)))

#define UNSET_UINT(uint_val, pos) ((uint_val) &= ~(1U << (pos)))

#define CLEAR_UINT(uint_val) ((uint_val) = 0)

#define FILL_UINT(uint_val) ((uint_val) = (~0U))