#pragma once

#include <cstdint>

constexpr uint16_t ring0_code_selector = 0x08;
constexpr uint16_t ring0_data_selector = 0x10;
constexpr uint16_t ring0_tss_selector = 0x18;
constexpr uint16_t ring3_code_selector = 0x23;
constexpr uint16_t ring3_data_selector = 0x2b; // also used from entry.asm
