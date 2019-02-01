#include "logo.hpp"
#include "util.hpp"

static const char logo[] =
  " |             _||       \n"
  "  _ \\  _|(_-<  _| _|  _| \n"
  "_.__/_|  ___/_| \\__|_|   \n";

static const char git_version[] =
#include "version.inc"
  ;

void print_logo()
{
  format(logo, "\nVersion ", git_version, "\n\n");
}
