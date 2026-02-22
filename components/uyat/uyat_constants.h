#pragma once

#include <cstddef>
#include "esphome/core/helpers.h"

namespace esphome::uyat {

// Buffer size for to_string conversions
static constexpr size_t UYAT_LOG_BUFFER_SIZE = 128u;
// Based on format_hex_pretty_size(UYAT_LOG_BUFFER_SIZE) + null terminator
static constexpr size_t UYAT_PRETTY_HEX_BUFFER_SIZE = format_hex_pretty_size(UYAT_LOG_BUFFER_SIZE) + 1;

}  // namespace esphome::uyat
