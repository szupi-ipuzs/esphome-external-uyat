#include <cstdint>
#include <optional>
#include <variant>
#include <string>
#include <vector>
#include <functional>
#include <cstdio>

#include "esphome/core/helpers.h"
#include "uyat_constants.h"

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

  void to_string(char* buffer, size_t size) const
  {
    int written = snprintf(buffer, size, "Datapoint %u: ", number);
    size_t pos = (written > 0) ? static_cast<size_t>(written) : 0u;
    if (pos >= size)
    {
      return;
    }

    if (types.empty())
    {
      snprintf(buffer + pos, size - pos, "ANY");
      return;
    }

    for (size_t i = 0; i < types.size(); ++i)
    {
      if (i > 0)
      {
        written = snprintf(buffer + pos, size - pos, ", ");
        if (written > 0)
        {
          pos += static_cast<size_t>(written);
          if (pos >= size)
          {
            return;
          }
        }
      }

      written = snprintf(buffer + pos, size - pos, "%s", get_type_name(types[i]));
      if (written > 0)
      {
        pos += static_cast<size_t>(written);
        if (pos >= size)
        {
          return;
        }
      }
    }
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

  void to_string(char* buffer, size_t size) const
  {
    size_t data_len = value.size();
    if (data_len > UYAT_LOG_BUFFER_SIZE)
    {
      data_len = UYAT_LOG_BUFFER_SIZE;
    }
    format_hex_pretty_to(buffer, size, value.data(), data_len);
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

  void to_string(char* buffer, size_t size) const
  {
    snprintf(buffer, size, "%s", TRUEFALSE(value));
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

  void to_string(char* buffer, size_t size) const
  {
    snprintf(buffer, size, "%u", value);
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
  std::string value;

  void to_string(char* buffer, size_t size) const
  {
    snprintf(buffer, size, "%s", value.c_str());
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

  void to_string(char* buffer, size_t size) const
  {
    snprintf(buffer, size, "%d", value);
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

  void to_string(char* buffer, size_t size) const
  {
    snprintf(buffer, size, "%08X", value);
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

  void value_to_string(char* buffer, size_t size) const
  {
    std::visit([buffer, size](const auto& dp){
      dp.to_string(buffer, size);
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

  void to_string(char* buffer, size_t size) const
  {
    char temp_value[UYAT_PRETTY_HEX_BUFFER_SIZE];
    value_to_string(temp_value, sizeof(temp_value));

    snprintf(buffer, size, "Datapoint %u: %s (value: %s)",
             number, get_type_name(), temp_value);
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
    if (raw_data[1] == static_cast<uint8_t>(UyatDatapointType::RAW))
    {
      return UyatDatapoint{raw_data[0], RawDatapointValue{std::vector<uint8_t>{payload, &payload[payload_size]}}};
    }
    if (raw_data[1] == static_cast<uint8_t>(UyatDatapointType::BOOLEAN))
    {
      if (payload_size != 1u)
      {
        return {};
      }
      return UyatDatapoint{raw_data[0], BoolDatapointValue{payload[0] != 0x00}};
    }
    if (raw_data[1] == static_cast<uint8_t>(UyatDatapointType::INTEGER))
    {
      if (payload_size != 4u)
      {
        return {};
      }
      return UyatDatapoint{raw_data[0], UIntDatapointValue{encode_uint32(payload[0], payload[1], payload[2], payload[3])}};
    }
    if (raw_data[1] == static_cast<uint8_t>(UyatDatapointType::STRING))
    {
      return UyatDatapoint{raw_data[0], StringDatapointValue{std::string(reinterpret_cast<const char *>(payload), reinterpret_cast<const char *>(&payload[payload_size]))}};
    }
    if (raw_data[1] == static_cast<uint8_t>(UyatDatapointType::ENUM))
    {
      if (payload_size != 1u)
      {
        return {};
      }
      return UyatDatapoint{raw_data[0], EnumDatapointValue{payload[0]}};
    }
    if (raw_data[1] == static_cast<uint8_t>(UyatDatapointType::BITMAP))
    {
      if (payload_size == 1u)
      {
        return UyatDatapoint{raw_data[0], BitmapDatapointValue{payload[0]}};
      }
      if (payload_size == 2u)
      {
        return UyatDatapoint{raw_data[0], BitmapDatapointValue{encode_uint16(payload[0], payload[1])}};
      }
      if (payload_size == 4u)
      {
        return UyatDatapoint{raw_data[0], BitmapDatapointValue{encode_uint32(payload[0], payload[1], payload[2], payload[3])}};
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