#pragma once

#include "compiler.h"
#include <stddef.h>

#define offsetof(type, field) __builtin_offsetof (type, field)
