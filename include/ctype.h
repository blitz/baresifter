#pragma once

static inline int isdigit(int c)
{
  return c >= '0' && c <= '9';
}

static inline int isspace(int c)
{
  switch (c) {
  case ' ':
  case '\t':
  case '\n':
    return 1;
  default:
    return 0;
  }
}
