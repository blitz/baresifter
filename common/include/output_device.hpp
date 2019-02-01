#pragma once

class output_device {
public:

  virtual void putc(char c) = 0;
  virtual void puts(const char *s);

  // Factory method.
  static output_device *make();
};
