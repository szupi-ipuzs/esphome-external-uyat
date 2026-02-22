#pragma once

#include "esphome/core/helpers.h"

#include <functional>
#include <string>
#include <optional>

#include "uyat_datapoint_types.h"

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

   using OnValueCallback = std::function<void(const std::string&)>;

   struct Config
   {
      MatchingDatapoint matching_dp;
      const TextDataEncoding data_encoding;

      void to_string(char* buffer, size_t size) const
      {
         char temp_dp[UYAT_LOG_BUFFER_SIZE];
         this->matching_dp.to_string(temp_dp, sizeof(temp_dp));
         snprintf(buffer, size, "%s %s",
                  TextDataEncoding2String(this->data_encoding),
                  temp_dp);
      }
   };

   void init(DatapointHandler& handler)
   {
      this->handler_ = &handler;
      this->handler_->register_datapoint_listener(this->config_.matching_dp, [this](const UyatDatapoint &datapoint) {
         char log_str[UYAT_PRETTY_HEX_BUFFER_SIZE];
         datapoint.to_string(log_str, sizeof(log_str));
         ESP_LOGV(DpText::TAG, "%s processing as text_sensor", log_str);

         if (!this->config_.matching_dp.matches(datapoint.get_type()))
         {
            datapoint.to_string(log_str, sizeof(log_str));
            ESP_LOGW(DpText::TAG, "Non-matching datapoint type %s!", log_str);
            return;
         }

         if (auto * dp_value = std::get_if<RawDatapointValue>(&datapoint.value))
         {
            if (!this->config_.matching_dp.allows_single_type())
            {
               this->config_.matching_dp.types = {UyatDatapointType::RAW};
               this->config_.matching_dp.to_string(log_str, sizeof(log_str));
               ESP_LOGI(DpText::TAG, "Resolved %s", log_str);
            }
            this->last_received_value_ = std::string(dp_value->value.begin(), dp_value->value.end());
            this->last_received_value_ = this->decode_(this->last_received_value_);
            callback_(this->last_received_value_);
         }
         else
         if (auto * dp_value = std::get_if<StringDatapointValue>(&datapoint.value))
         {
            if (!this->config_.matching_dp.allows_single_type())
            {
               this->config_.matching_dp.types = {UyatDatapointType::STRING};
               this->config_.matching_dp.to_string(log_str, sizeof(log_str));
               ESP_LOGI(DpText::TAG, "Resolved %s", log_str);
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

   std::string get_last_received_value() const
   {
      return last_received_value_;
   }

   std::string get_last_set_value() const
   {
      return last_set_value_;
   }

   const Config& get_config() const
   {
      return config_;
   }

   void set_value(const std::string& value)
   {
      char config_str[UYAT_LOG_BUFFER_SIZE];
      
      if (this->handler_ == nullptr)
      {
         this->config_.to_string(config_str, sizeof(config_str));
         ESP_LOGE(DpText::TAG, "DatapointHandler not initialized for %s", config_str);
         return;
      }

      if (value.empty())
      {
         this->config_.to_string(config_str, sizeof(config_str));
         ESP_LOGW(DpText::TAG, "Not setting empty value for %s", config_str);
         return;
      }

      this->config_.to_string(config_str, sizeof(config_str));
      ESP_LOGV(DpText::TAG, "Setting value to %s for %s", value.c_str(), config_str);
      this->last_set_value_ = value;

      if (!this->config_.matching_dp.allows_single_type())
      {
         this->config_.matching_dp.to_string(config_str, sizeof(config_str));
         ESP_LOGW(DpText::TAG, "Cannot set value, datapoint type not yet known for %s", config_str);
         return;
      }

      std::optional<UyatDatapoint> to_set_dp;
      if (this->config_.data_encoding == TextDataEncoding::AS_BASE64)
      {
         std::string encoded = base64_encode(reinterpret_cast<const uint8_t*>(value.data()), value.size());
         if (encoded.empty())
         {
            this->config_.to_string(config_str, sizeof(config_str));
            ESP_LOGW(DpText::TAG, "Not setting invalid base64 value for %s", config_str);
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
            this->config_.to_string(config_str, sizeof(config_str));
            ESP_LOGW(DpText::TAG, "Not setting invalid hex value for %s", config_str);
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
               StringDatapointValue{std::string(parsed.begin(), parsed.end())}
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
         this->config_.to_string(config_str, sizeof(config_str));
         ESP_LOGW(DpText::TAG, "Not setting empty (after encoding) value for %s", config_str);
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

   std::string decode_(const std::string& input) const
   {
      if (input.empty())
      {
         return {};
      }

      if (this->config_.data_encoding == TextDataEncoding::AS_BASE64)
      {
         const auto decoded = base64_decode(input);
         if (decoded.empty())
         {
            return {};
         }
         return std::string(decoded.begin(), decoded.end());
      }

      if (this->config_.data_encoding == TextDataEncoding::AS_HEX)
      {
         return format_hex_pretty(input.data(), input.size(), 0);
      }

      return input;
   }

   Config config_;
   OnValueCallback callback_;

   DatapointHandler* handler_{nullptr};
   std::string last_received_value_;
   std::string last_set_value_;
};

}
