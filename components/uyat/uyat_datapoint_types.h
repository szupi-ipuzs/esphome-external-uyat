#include <cstdint>
#include <deque>
#include <optional>
#include <variant>
#include <string>
#include <vector>
#include <functional>

#include "esphome/core/helpers.h"
#include "uyat_string.hpp"
#include "uyat_deque.hpp"

#pragma once

namespace esphome::uyat
{

// this is the actual value sent to the mcu
enum class UyatDatapointType: uint8_t {
  RAW = 0x00,      // variable length
  BOOLEAN = 0x01,  // 1 byte (0/1)
  INTEGER = 0x02,  // 4 byte
  STRING = 0x03,   // variable length
  ENUM = 0x04,     // 1 byte
  BITMAP = 0x05,  // 1/2/4 bytes
};

struct MatchingDatapoint
{
  uint8_t number;
  std::vector<UyatDatapointType> types;

  static constexpr const char* get_type_name(const UyatDatapointType dp_type)
  {
    switch(dp_type)
    {
      case UyatDatapointType::RAW:
        return "RAW";
      case UyatDatapointType::BOOLEAN:
        return "BOOL";
      case UyatDatapointType::INTEGER:
        return "INTEGER";
      case UyatDatapointType::STRING:
        return "STRING";
      case UyatDatapointType::ENUM:
        return "ENUM";
      case UyatDatapointType::BITMAP:
        return "BITMAP";
      default:
        return "UNKNOWN";
    }
  }

  StaticString to_string() const
  {
    StaticString type_list;
    if (types.empty())
    {
      type_list = "ANY";
    }
    else
    {
      for (const auto& type : types)
      {
        if (!type_list.empty())
        {
          type_list += ", ";
        }
        type_list += get_type_name(type);
      }
    }
    return StringHelpers::sprintf("Datapoint %u:", number) + type_list;
  }

  bool matches(const UyatDatapointType dp_type) const
  {
    if (types.empty())
    {
      return true;
    }

    for (const auto& type : types)
    {
      if (type == dp_type)
      {
        return true;
      }
    }
    return false;
  }

  bool allows_single_type() const
  {
    return types.size() == 1;
  }

  bool allows_any_type() const
  {
    return types.empty();
  }
};

struct RawDatapointValue {
  static constexpr UyatDatapointType dp_type = UyatDatapointType::RAW;
  std::vector<uint8_t> value;

  StaticString to_string() const
  {
    return StringHelpers::format_hex_pretty(value);
  }

  std::vector<uint8_t> to_payload() const
  {
    return value;
  }

  bool operator==(const RawDatapointValue& other) const
  {
    return value == other.value;
  }
};

struct BoolDatapointValue {
  static constexpr UyatDatapointType dp_type = UyatDatapointType::BOOLEAN;
  bool value;

  StaticString to_string() const
  {
    return TRUEFALSE(value);
  }

  std::vector<uint8_t> to_payload() const
  {
    return std::vector<uint8_t>{static_cast<uint8_t>(value? 0x01 : 0x00)};
  }

  bool operator==(const BoolDatapointValue& other) const
  {
    return value == other.value;
  }
};

struct UIntDatapointValue {
  static constexpr UyatDatapointType dp_type = UyatDatapointType::INTEGER;
  uint32_t value;

  StaticString to_string() const
  {
    return StringHelpers::sprintf("%u", value);
  }

  std::vector<uint8_t> to_payload() const
  {
    return std::vector<uint8_t>{
      static_cast<uint8_t>(value >> 24),
      static_cast<uint8_t>(value >> 16),
      static_cast<uint8_t>(value >> 8),
      static_cast<uint8_t>(value >> 0),
    };
  }

  bool operator==(const UIntDatapointValue& other) const
  {
    return value == other.value;
  }
};

struct StringDatapointValue {
  static constexpr UyatDatapointType dp_type = UyatDatapointType::STRING;
  StaticString value;

  StaticString to_string() const
  {
    return value;
  }

  std::vector<uint8_t> to_payload() const
  {
    std::vector<uint8_t> data;
    data.reserve(value.size());
    for (char const &c : value) {
      data.push_back(c);
    }

    return data;
  }

  bool operator==(const StringDatapointValue& other) const
  {
    return value == other.value;
  }
};

struct EnumDatapointValue {
  static constexpr UyatDatapointType dp_type = UyatDatapointType::ENUM;
  uint8_t value;

  StaticString to_string() const
  {
    return StringHelpers::sprintf("%d", value);
  }

  std::vector<uint8_t> to_payload() const
  {
    return std::vector<uint8_t>{value};
  }

  bool operator==(const EnumDatapointValue& other) const
  {
    return value == other.value;
  }
};

struct BitmapDatapointValue {
  static constexpr UyatDatapointType dp_type = UyatDatapointType::BITMAP;
  uint32_t value;

  StaticString to_string() const
  {
    return StringHelpers::sprintf("%08X", value);
  }

  std::vector<uint8_t> to_payload() const
  {
    // choose size based on highest set bit
    if (value <= 0xFFu)
    {
      return std::vector<uint8_t>{
        static_cast<uint8_t>(value),
      };
    }
    else if (value <= 0xFFFFu)
    {
      return std::vector<uint8_t>{
        static_cast<uint8_t>(value >> 8),
        static_cast<uint8_t>(value >> 0),
      };
    }
    else if (value <= 0xFFFFFFFFu)
    {
      return std::vector<uint8_t>{
        static_cast<uint8_t>(value >> 24),
        static_cast<uint8_t>(value >> 16),
        static_cast<uint8_t>(value >> 8),
        static_cast<uint8_t>(value >> 0),
      };
    }
    return {};
  }

  bool operator==(const BitmapDatapointValue& other) const
  {
    return value == other.value;
  }
};

using AnyDatapointValue = std::variant<RawDatapointValue,
                                       BoolDatapointValue,
                                       UIntDatapointValue,
                                       StringDatapointValue,
                                       EnumDatapointValue,
                                       BitmapDatapointValue>;

struct UyatDatapoint {
  uint8_t number;
  AnyDatapointValue value;

  bool matches(const UyatDatapoint& other) const
  {
    if (other.number != number)
    {
      return false;
    }

    return (other.get_type() == get_type());
  }

  bool matches(const MatchingDatapoint& matching) const
  {
    return (matching.number == number) && (matching.matches(get_type()));
  }

  constexpr UyatDatapointType get_type() const
  {
    return std::visit([](const auto& dp){
      return std::decay_t<decltype(dp)>::dp_type;
    },
    value);
  }

  constexpr const char* get_type_name() const
  {
    return MatchingDatapoint::get_type_name(get_type());
  }

  StaticString value_to_string() const
  {
    return std::visit([](const auto& dp){
      return dp.to_string();
    },
    value);
  }

  std::vector<uint8_t> value_to_payload() const
  {
    return std::visit([](const auto& dp){
      return dp.to_payload();
    },
    value);
  }

  StaticString to_string() const
  {
    return StringHelpers::sprintf("Datapoint %u: %s (value: %s)", number, get_type_name(), value_to_string().c_str());
  }

  static std::optional<UyatDatapoint> construct(const StaticDeque::DequeView &raw_data, std::size_t &used_len)
  {
    used_len = 0;
    if (raw_data.size_ < 4u)
    {
      used_len = raw_data.size_;
      return {};
    }

    size_t payload_size = (raw_data.byte_at(2u) << 8) + raw_data.byte_at(3u);
    if ((0u == payload_size) || (payload_size > (raw_data.size_ - 4u)))
    {
      used_len = raw_data.size_;
      return {};
    }

    used_len = payload_size + 4u;
    const uint8_t dp_type = raw_data.byte_at(1u);
    const uint8_t dp_number = raw_data.byte_at(0u);
    const auto payload = raw_data.create_view(4u, payload_size);

    if (dp_type == static_cast<uint8_t>(UyatDatapointType::RAW))
    {
      return UyatDatapoint{dp_number, RawDatapointValue{std::vector<uint8_t>(payload.cbegin(), payload.cend())}};
    }
    if (dp_type == static_cast<uint8_t>(UyatDatapointType::BOOLEAN))
    {
      if (payload.size_ != 1u)
      {
        return {};
      }
      return UyatDatapoint{dp_number, BoolDatapointValue{raw_data.byte_at(0) != 0x00}};
    }
    if (dp_type == static_cast<uint8_t>(UyatDatapointType::INTEGER))
    {
      if (payload.size_ != 4u)
      {
        return {};
      }
      return UyatDatapoint{dp_number,
                           UIntDatapointValue{encode_uint32(raw_data.byte_at(0), raw_data.byte_at(1u),
                                                           raw_data.byte_at(2u), raw_data.byte_at(3u))}};
    }
    if (dp_type == static_cast<uint8_t>(UyatDatapointType::STRING))
    {
      StaticString payload_str;
      payload_str.reserve(payload.size_);
      for (size_t i = 0; i < payload.size_; ++i) {
        payload_str.push_back(static_cast<char>(raw_data.byte_at(i)));
      }
      return UyatDatapoint{dp_number, StringDatapointValue{std::move(payload_str)}};
    }
    if (dp_type == static_cast<uint8_t>(UyatDatapointType::ENUM))
    {
      if (payload.size_ != 1u)
      {
        return {};
      }
      return UyatDatapoint{dp_number, EnumDatapointValue{raw_data.byte_at(0)}};
    }
    if (dp_type == static_cast<uint8_t>(UyatDatapointType::BITMAP))
    {
      if (payload.size_ == 1u)
      {
        return UyatDatapoint{dp_number, BitmapDatapointValue{raw_data.byte_at(0)}};
      }
      if (payload.size_ == 2u)
      {
        return UyatDatapoint{dp_number,
                             BitmapDatapointValue{encode_uint16(raw_data.byte_at(0), raw_data.byte_at(1u))}};
      }
      if (payload.size_ == 4u)
      {
        return UyatDatapoint{dp_number,
                             BitmapDatapointValue{encode_uint32(raw_data.byte_at(0), raw_data.byte_at(1u),
                                                               raw_data.byte_at(2u), raw_data.byte_at(3u))}};
      }
    }

    return {};
  }
};

using OnDatapointCallback = std::function<void(UyatDatapoint)>;

struct DatapointHandler
{
  virtual ~DatapointHandler() = default;

  virtual void register_datapoint_listener(const MatchingDatapoint& matching_dp, const OnDatapointCallback& callback) = 0;
  virtual void set_datapoint_value(const UyatDatapoint& dp, const bool forced = false) = 0;
};

}