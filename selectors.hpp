#pragma once

#include <cstdint>

constexpr uint16_t ring0_code_selector = 0x10;
constexpr uint16_t ring0_data_selector = 0x20;
constexpr uint16_t ring0_tss_selector = 0x30;
constexpr uint16_t ring3_code_selector = 0x40;
constexpr uint16_t ring3_data_selector = 0x50;
