#pragma once

#include <cstdint>

struct execution_attempt {
  uint8_t length = 0;
  uint8_t exception = 0;
};

inline bool operator==(execution_attempt const &a, execution_attempt const &b)
{
  return a.length == b.length and a.exception == b.exception;
}

inline bool operator!=(execution_attempt const &a, execution_attempt const &b)
{
  return not (a == b);
}
