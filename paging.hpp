#pragma once

#include <cstddef>

const size_t page_size = 4096;

// The user space page as a read-only user-accessible mapping.
uintptr_t get_user_page();

// The user space page as a read-write supervisor-accessible mapping.
char *get_user_page_backing();

void setup_paging();
