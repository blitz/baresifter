#pragma once

#include "exception_frame.hpp"

extern "C" void start();

void setup_idt();
exception_frame execute_user(uintptr_t rip);
