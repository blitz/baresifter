#pragma once

void setup_idt();

extern "C" [[noreturn]] void irq_entry();
