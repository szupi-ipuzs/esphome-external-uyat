#pragma once
#include "esphome/core/helpers.h"
#include <vector>
#include "sma_stl.hpp"
#include <cstring>
#include <cstdio>

namespace esphome::uyat
{

static constexpr const std::size_t MAX_STRING_BUFFER_SIZE = 1024u * 5u;
static constexpr const std::size_t MAX_STRING_BUFFER_SLOTS = 500u;

struct StringMemoryPool
{
   StringMemoryPool(const std::size_t max_buffer_size, const std::size_t max_buffer_chunks):
   buffer_(max_buffer_size),
   allocator_(buffer_, max_buffer_chunks)
   {}

   static sma::StaticMemoryAllocator& get_sma()
   {
      static StringMemoryPool instance(MAX_STRING_BUFFER_SIZE, MAX_STRING_BUFFER_SLOTS);
      return instance.allocator_;
   }

private:

   std::vector<uint8_t> buffer_;
   sma::StaticMemoryAllocator allocator_;
};

using StaticString = std::basic_string<char, std::char_traits<char>, sma::STLAllocator<char, StringMemoryPool>>;

struct StringHelpers
{
   static StaticString sprintf(const char *fmt, ...)
   {
      StaticString str;
      va_list args;

      va_start(args, fmt);
      size_t length = vsnprintf(nullptr, 0, fmt, args);
      va_end(args);

      str.resize(length);
      va_start(args, fmt);
      vsnprintf(&str[0], length + 1, fmt, args);
      va_end(args);

      return str;
   }

   static StaticString format_hex_pretty(const uint8_t *data, size_t length, char separator = '.', bool show_length = true)
   {
      if (data == nullptr || length == 0)
         return "";
      StaticString ret;
      size_t hex_len = separator ? (length * 3 - 1) : (length * 2);
      ret.resize(hex_len);
      ::esphome::format_hex_pretty_to(&ret[0], hex_len + 1, data, length, separator);
      if (show_length && length > 4)
         return ret + " (" + std::to_string(length).c_str() + ")";
      return ret;
   }

   static StaticString format_hex_pretty(const std::vector<uint8_t> &data, char separator = '.', bool show_length = true)
   {
      return StringHelpers::format_hex_pretty(data.data(), data.size(), separator, show_length);
   }

   static std::vector<uint8_t> base64_decode(const StaticString &encoded_string)
   {
      // Calculate maximum decoded size: every 4 base64 chars = 3 bytes
      size_t max_len = ((encoded_string.size() + 3) / 4) * 3;
      std::vector<uint8_t> ret(max_len);
      size_t actual_len = ::esphome::base64_decode(reinterpret_cast<const uint8_t *>(encoded_string.c_str()), encoded_string.length(), ret.data(), max_len);
      ret.resize(actual_len);
      return ret;
   }
};

}
