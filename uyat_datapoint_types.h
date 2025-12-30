#include <cstdint>
#include <optional>
#include <variant>
#include <string>
#include <vector>
#include <functional>

#include "esphome/core/helpers.h"

#pragma once

namespace esphome::uyat
{

// this is the actual value sent to the mcu
enum UyatDatapointTypeInternal: uint8_t {
  RAW = 0x00,      // variable length
  BOOLEAN = 0x01,  // 1 byte (0/1)
  INTEGER = 0x02,  // 4 byte
  STRING = 0x03,   // variable length
  ENUM = 0x04,     // 1 byte
  BITMASK = 0x05,  // 1/2/4 bytes
};

enum class UyatDatapointType {
  RAW,
  BOOLEAN,
  INTEGER,
  STRING,
  ENUM,
  BITMASK8,
  BITMASK16,
  BITMASK32,
};

struct MatchingDatapoint
{
  uint8_t number;
  std::optional<UyatDatapointType> type;  // empty means any

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
      case UyatDatapointType::BITMASK8:
        return "BITMASK8";
      case UyatDatapointType::BITMASK16:
        return "BITMASK16";
      case UyatDatapointType::BITMASK32:
        return "BITMASK32";
      default:
        return "UNKNOWN";
    }
  }

  std::string to_string() const
  {
    if (type)
    {
      return str_sprintf("Datapoint %u: %s", number, get_type_name(type.value()));
    }
    else
    {
      return str_sprintf("Datapoint %u: ANY", number);
    }
  }
};

struct RawDatapointValue {
  static constexpr UyatDatapointType dp_type = UyatDatapointType::RAW;
  static constexpr UyatDatapointTypeInternal internal_dp_type = UyatDatapointTypeInternal::RAW;
  std::vector<uint8_t> value;

  std::string to_string() const
  {
    return format_hex_pretty(value);
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
  static constexpr UyatDatapointTypeInternal internal_dp_type = UyatDatapointTypeInternal::BOOLEAN;
  bool value;

  std::string to_string() const
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
  static constexpr UyatDatapointTypeInternal internal_dp_type = UyatDatapointTypeInternal::INTEGER;
  uint32_t value;

  std::string to_string() const
  {
    return str_sprintf("%u", value);
  }

  std::vector<uint8_t> to_payload() const
  {
    const auto converted = convert_big_endian(value);
    return std::vector<uint8_t>{
      static_cast<uint8_t>(converted >> 24),
      static_cast<uint8_t>(converted >> 16),
      static_cast<uint8_t>(converted >> 8),
      static_cast<uint8_t>(converted >> 0),
    };
  }

  bool operator==(const UIntDatapointValue& other) const
  {
    return value == other.value;
  }
};

struct StringDatapointValue {
  static constexpr UyatDatapointType dp_type = UyatDatapointType::STRING;
  static constexpr UyatDatapointTypeInternal internal_dp_type = UyatDatapointTypeInternal::STRING;
  std::string value;

  std::string to_string() const
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
  static constexpr UyatDatapointTypeInternal internal_dp_type = UyatDatapointTypeInternal::ENUM;
  uint8_t value;

  std::string to_string() const
  {
    return str_sprintf("%d", value);
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

struct Bitmask8DatapointValue {
  static constexpr UyatDatapointType dp_type = UyatDatapointType::BITMASK8;
  static constexpr UyatDatapointTypeInternal internal_dp_type = UyatDatapointTypeInternal::BITMASK;
  uint8_t value;

  std::string to_string() const
  {
    return str_sprintf("%02X", value);
  }

  std::vector<uint8_t> to_payload() const
  {
    return std::vector<uint8_t>{value};
  }

  bool operator==(const Bitmask8DatapointValue& other) const
  {
    return value == other.value;
  }
};

struct Bitmask16DatapointValue {
  static constexpr UyatDatapointType dp_type = UyatDatapointType::BITMASK16;
  static constexpr UyatDatapointTypeInternal internal_dp_type = UyatDatapointTypeInternal::BITMASK;
  uint16_t value;

  std::string to_string() const
  {
    return str_sprintf("%08X", value);
  }

  std::vector<uint8_t> to_payload() const
  {
    const auto converted = convert_big_endian(value);
    return std::vector<uint8_t>{
      static_cast<uint8_t>(converted >> 8),
      static_cast<uint8_t>(converted >> 0),
    };
  }

  bool operator==(const Bitmask16DatapointValue& other) const
  {
    return value == other.value;
  }
};

struct Bitmask32DatapointValue {
  static constexpr UyatDatapointType dp_type = UyatDatapointType::BITMASK32;
  static constexpr UyatDatapointTypeInternal internal_dp_type = UyatDatapointTypeInternal::BITMASK;
  uint32_t value;

  std::string to_string() const
  {
    return str_sprintf("%08X", value);
  }

  std::vector<uint8_t> to_payload() const
  {
    const auto converted = convert_big_endian(value);
    return std::vector<uint8_t>{
      static_cast<uint8_t>(converted >> 24),
      static_cast<uint8_t>(converted >> 16),
      static_cast<uint8_t>(converted >> 8),
      static_cast<uint8_t>(converted >> 0),
    };
  }

  bool operator==(const Bitmask32DatapointValue& other) const
  {
    return value == other.value;
  }
};

using AnyDatapointValue = std::variant<RawDatapointValue,
                                       BoolDatapointValue,
                                       UIntDatapointValue,
                                       StringDatapointValue,
                                       EnumDatapointValue,
                                       Bitmask8DatapointValue,
                                       Bitmask16DatapointValue,
                                       Bitmask32DatapointValue>;

struct UyatDatapoint {
  uint8_t number;
  AnyDatapointValue value;

  MatchingDatapoint make_matching() const
  {
    return MatchingDatapoint{number, get_type()};
  }

  bool matches(const UyatDatapoint& other) const
  {
    if (other.number != number)
    {
      return false;
    }

    return (other.get_internal_type() == get_internal_type());
  }

  bool matches(const MatchingDatapoint& matching) const
  {
    if (matching.number != number)
    {
      return false;
    }

    return (!matching.type) || (matching.type == get_type());
  }

  constexpr UyatDatapointType get_type() const
  {
    return std::visit([](const auto& dp){
      return std::decay_t<decltype(dp)>::dp_type;
    },
    value);
  }

  constexpr UyatDatapointTypeInternal get_internal_type() const
  {
    return std::visit([](const auto& dp){
      return std::decay_t<decltype(dp)>::internal_dp_type;
    },
    value);
  }

  constexpr const char* get_type_name() const
  {
    return MatchingDatapoint::get_type_name(get_type());
  }

  std::string value_to_string() const
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

  std::string to_string() const
  {
    return str_sprintf("Datapoint %u: %s (value: %s)", number, get_type_name(), value_to_string().c_str());
  }

  static std::optional<UyatDatapoint> construct(const uint8_t* raw_data, const std::size_t raw_data_len, std::size_t& used_len)
  {
    used_len = 0;
    if (raw_data_len < 4u)
    {
      used_len = raw_data_len;
      return {};
    }

    size_t payload_size = (raw_data[2] << 8) + raw_data[3];
    if ((0u == payload_size) || (payload_size > (raw_data_len - 4u)))
    {
      used_len = raw_data_len;
      return {};
    }

    const uint8_t *payload = &raw_data[4u];

    used_len = payload_size + 4u;
    if (raw_data[1] == UyatDatapointTypeInternal::RAW)
    {
      return UyatDatapoint{raw_data[0], RawDatapointValue{std::vector<uint8_t>{payload, &payload[payload_size]}}};
    }
    if (raw_data[1] == UyatDatapointTypeInternal::BOOLEAN)
    {
      if (payload_size != 1u)
      {
        return {};
      }
      return UyatDatapoint{raw_data[0], BoolDatapointValue{payload[0] != 0x00}};
    }
    if (raw_data[1] == UyatDatapointTypeInternal::INTEGER)
    {
      if (payload_size != 4u)
      {
        return {};
      }
      return UyatDatapoint{raw_data[0], UIntDatapointValue{encode_uint32(payload[0], payload[1], payload[2], payload[3])}};
    }
    if (raw_data[1] == UyatDatapointTypeInternal::STRING)
    {
      return UyatDatapoint{raw_data[0], StringDatapointValue{std::string(reinterpret_cast<const char *>(payload), reinterpret_cast<const char *>(&payload[payload_size]))}};
    }
    if (raw_data[1] == UyatDatapointTypeInternal::ENUM)
    {
      if (payload_size != 1u)
      {
        return {};
      }
      return UyatDatapoint{raw_data[0], EnumDatapointValue{payload[0]}};
    }
    if (raw_data[1] == UyatDatapointTypeInternal::BITMASK)
    {
      if (payload_size == 1u)
      {
        return UyatDatapoint{raw_data[0], Bitmask8DatapointValue{payload[0]}};
      }
      if (payload_size == 2u)
      {
        return UyatDatapoint{raw_data[0], Bitmask16DatapointValue{encode_uint16(payload[0], payload[1])}};
      }
      if (payload_size == 4u)
      {
        return UyatDatapoint{raw_data[0], Bitmask32DatapointValue{encode_uint32(payload[0], payload[1], payload[2], payload[3])}};
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