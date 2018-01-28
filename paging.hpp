#pragma once

#include <cstddef>

const size_t page_size = 4096;

char *get_user_page();
void setup_paging();
