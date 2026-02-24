#pragma once

#include "esphome/core/helpers.h"

#include <functional>
#include <string>
#include <optional>

#include "uyat_datapoint_types.h"
#include "uyat_string.hpp"

namespace esphome::uyat
{

enum class TextDataEncoding
{
   PLAIN,
   AS_BASE64,
   AS_HEX
};

static constexpr const char* TextDataEncoding2String(const TextDataEncoding encoding)
{
   switch(encoding)
   {
      case TextDataEncoding::PLAIN:
         return "Plain";
      case TextDataEncoding::AS_BASE64:
         return "Base64";
      case TextDataEncoding::AS_HEX:
         return "Hex";
      default:
         return "Unknown";
   }
}

struct DpText
{
   static constexpr const char * TAG = "uyat.DpText";

   using OnValueCallback = std::function<void(const String&)>;

   struct Config
   {
      MatchingDatapoint matching_dp;
      const TextDataEncoding data_encoding;

      const String to_string() const
      {
         return StringHelpers::sprintf("%s %s", TextDataEncoding2String(this->data_encoding), this->matching_dp.to_string().c_str());
      }
   };

   void init(DatapointHandler& handler)
   {
      this->handler_ = &handler;
      this->handler_->register_datapoint_listener(this->config_.matching_dp, [this](const UyatDatapoint &datapoint) {
         ESP_LOGV(DpText::TAG, "%s processing as text_sensor", datapoint.to_string().c_str());

         if (!this->config_.matching_dp.matches(datapoint.get_type()))
         {
            ESP_LOGW(DpText::TAG, "Non-matching datapoint type %s!", datapoint.get_type_name());
            return;
         }

         if (auto * dp_value = std::get_if<RawDatapointValue>(&datapoint.value))
         {
            if (!this->config_.matching_dp.allows_single_type())
            {
               this->config_.matching_dp.types = {UyatDatapointType::RAW};
               ESP_LOGI(DpText::TAG, "Resolved %s", this->config_.matching_dp.to_string().c_str());
            }
            this->last_received_value_ = String(dp_value->value.begin(), dp_value->value.end());
            this->last_received_value_ = this->decode_(this->last_received_value_);
            callback_(this->last_received_value_);
         }
         else
         if (auto * dp_value = std::get_if<StringDatapointValue>(&datapoint.value))
         {
            if (!this->config_.matching_dp.allows_single_type())
            {
               this->config_.matching_dp.types = {UyatDatapointType::STRING};
               ESP_LOGI(DpText::TAG, "Resolved %s", this->config_.matching_dp.to_string().c_str());
            }
            this->last_received_value_ = this->decode_(dp_value->value);
            callback_(this->last_received_value_);
         }
         else
         {
            ESP_LOGW(DpText::TAG, "Unhandled datapoint type %s!", datapoint.get_type_name());
            return;
         }
      });
   }

   String get_last_received_value() const
   {
      return last_received_value_;
   }

   String get_last_set_value() const
   {
      return last_set_value_;
   }

   const Config& get_config() const
   {
      return config_;
   }

   void set_value(const String& value)
   {
      if (this->handler_ == nullptr)
      {
         ESP_LOGE(DpText::TAG, "DatapointHandler not initialized for %s", this->config_.to_string().c_str());
         return;
      }

      if (value.empty())
      {
         ESP_LOGW(DpText::TAG, "Not setting empty value for %s", this->config_.to_string().c_str());
         return;
      }

      ESP_LOGV(DpText::TAG, "Setting value to %s for %s", value.c_str(), this->config_.to_string().c_str());
      this->last_set_value_ = value;

      if (!this->config_.matching_dp.allows_single_type())
      {
         ESP_LOGW(DpText::TAG, "Cannot set value, datapoint type not yet known for %s", this->config_.matching_dp.to_string().c_str());
         return;
      }

      std::optional<UyatDatapoint> to_set_dp;
      if (this->config_.data_encoding == TextDataEncoding::AS_BASE64)
      {
         // fixme: all flavours of base64_encode() return std::string
         //        we can copy this method to uyat StringHelpers, but it's a bit too complex
         //        better to wait till helpers provide a version that works with plane char buffers
         String encoded = base64_encode(reinterpret_cast<const uint8_t*>(value.data()), value.size()).c_str();
         if (encoded.empty())
         {
            ESP_LOGW(DpText::TAG, "Not setting invalid base64 value for %s", this->config_.to_string().c_str());
            return;
         }

         if (this->config_.matching_dp.matches(UyatDatapointType::RAW))
         {
            to_set_dp = UyatDatapoint{
               this->config_.matching_dp.number,
               RawDatapointValue{std::vector<uint8_t>(encoded.begin(), encoded.end())}
            };
         }
         if (this->config_.matching_dp.matches(UyatDatapointType::STRING))
         {
            to_set_dp = UyatDatapoint{
               this->config_.matching_dp.number,
               StringDatapointValue{encoded}
            };
         }
      }
      else
      if (this->config_.data_encoding == TextDataEncoding::AS_HEX)
      {
         std::vector<uint8_t> parsed;
         if ((!parse_hex(value.c_str(), parsed, value.size()/2)) || (parsed.empty()))
         {
            ESP_LOGW(DpText::TAG, "Not setting invalid hex value for %s", this->config_.to_string().c_str());
            return;
         }

         if (this->config_.matching_dp.matches(UyatDatapointType::RAW))
         {
            to_set_dp = UyatDatapoint{
               this->config_.matching_dp.number,
               RawDatapointValue{parsed}
            };
         }
         if (this->config_.matching_dp.matches(UyatDatapointType::STRING))
         {
            to_set_dp = UyatDatapoint{
               this->config_.matching_dp.number,
               StringDatapointValue{String(parsed.begin(), parsed.end())}
            };
         }
      }
      else
      {
         if (this->config_.matching_dp.matches(UyatDatapointType::RAW))
         {
            to_set_dp = UyatDatapoint{
               this->config_.matching_dp.number,
               RawDatapointValue{std::vector<uint8_t>(value.begin(), value.end())}
            };
         }
         if (this->config_.matching_dp.matches(UyatDatapointType::STRING))
         {
            to_set_dp = UyatDatapoint{
               this->config_.matching_dp.number,
               StringDatapointValue{value}
            };
         }
      }

      if (!to_set_dp.has_value())
      {
         ESP_LOGW(DpText::TAG, "Not setting empty (after encoding) value for %s", this->config_.to_string().c_str());
         return;
      }

      this->handler_->set_datapoint_value(to_set_dp.value());
   }

   DpText(DpText&&) = default;
   DpText& operator=(DpText&&) = default;

   DpText(const OnValueCallback& callback, MatchingDatapoint&& matching_dp, const TextDataEncoding data_encoding):
   config_{std::move(matching_dp), data_encoding},
   callback_(callback)
   {}

private:

   String decode_(const String& input) const
   {
      if (input.empty())
      {
         return {};
      }

      if (this->config_.data_encoding == TextDataEncoding::AS_BASE64)
      {
         const auto decoded = StringHelpers::base64_decode(input);
         if (decoded.empty())
         {
            return {};
         }
         return String(decoded.begin(), decoded.end());
      }

      if (this->config_.data_encoding == TextDataEncoding::AS_HEX)
      {
         return StringHelpers::format_hex_pretty(reinterpret_cast<const uint8_t*>(input.c_str()), input.length(), 0);
      }

      return input;
   }

   Config config_;
   OnValueCallback callback_;

   DatapointHandler* handler_{nullptr};
   String last_received_value_;
   String last_set_value_;
};

}
