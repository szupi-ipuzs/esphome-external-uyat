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

   void init(DatapointHandler& handler)
   {
      this->handler_ = &handler;
      this->handler_->register_datapoint_listener(this->matching_dp_, [this](const UyatDatapoint &datapoint) {
         ESP_LOGV(DpText::TAG, "%s processing as text_sensor", datapoint.to_string().c_str());

         if (!matching_dp_.matches(datapoint.get_type()))
         {
            ESP_LOGW(DpText::TAG, "Non-matching datapoint type %s!", datapoint.get_type_name());
            return;
         }

         if (auto * dp_value = std::get_if<RawDatapointValue>(&datapoint.value))
         {
            if (!this->matching_dp_.allows_single_type())
            {
               this->matching_dp_.types = {UyatDatapointType::RAW};
               ESP_LOGI(DpText::TAG, "Resolved %s", this->matching_dp_.to_string().c_str());
            }
            this->received_value_ = std::string(dp_value->value.begin(), dp_value->value.end());
            this->received_value_ = this->decode_(this->received_value_);
            callback_(this->received_value_);
         }
         else
         if (auto * dp_value = std::get_if<StringDatapointValue>(&datapoint.value))
         {
            if (!this->matching_dp_.allows_single_type())
            {
               this->matching_dp_.types = {UyatDatapointType::STRING};
               ESP_LOGI(DpText::TAG, "Resolved %s", this->matching_dp_.to_string().c_str());
            }
            this->received_value_ = this->decode_(dp_value->value);
            callback_(this->received_value_);
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
      return received_value_;
   }

   std::string get_last_set_value() const
   {
      return set_value_;
   }

   const std::string config_to_string() const
   {
      return str_sprintf("%s %s", TextDataEncoding2String(this->data_encoding_), this->matching_dp_.to_string().c_str());
   }

   void set_value(const std::string& value)
   {
      if (this->handler_ == nullptr)
      {
         ESP_LOGE(DpText::TAG, "DatapointHandler not initialized for %s", this->config_to_string().c_str());
         return;
      }

      if (value.empty())
      {
         ESP_LOGW(DpText::TAG, "Not setting empty value for %s", this->config_to_string().c_str());
         return;
      }

      ESP_LOGV(DpText::TAG, "Setting value to %s for %s", value.c_str(), this->config_to_string().c_str());
      this->set_value_ = value;

      if (!this->matching_dp_.allows_single_type())
      {
         ESP_LOGW(DpText::TAG, "Cannot set value, datapoint type not yet known for %s", this->matching_dp_.to_string().c_str());
         return;
      }

      std::optional<UyatDatapoint> to_set_dp;
      if (this->data_encoding_ == TextDataEncoding::AS_BASE64)
      {
         std::string encoded = base64_encode(reinterpret_cast<const uint8_t*>(value.data()), value.size());
         if (encoded.empty())
         {
            ESP_LOGW(DpText::TAG, "Not setting invalid base64 value for %s", this->config_to_string().c_str());
            return;
         }

         if (this->matching_dp_.matches(UyatDatapointType::RAW))
         {
            to_set_dp = UyatDatapoint{
               this->matching_dp_.number,
               RawDatapointValue{std::vector<uint8_t>(encoded.begin(), encoded.end())}
            };
         }
         if (this->matching_dp_.matches(UyatDatapointType::STRING))
         {
            to_set_dp = UyatDatapoint{
               this->matching_dp_.number,
               StringDatapointValue{encoded}
            };
         }
      }
      else
      if (this->data_encoding_ == TextDataEncoding::AS_HEX)
      {
         std::vector<uint8_t> parsed;
         if ((!parse_hex(value.c_str(), parsed, value.size()/2)) || (parsed.empty()))
         {
            ESP_LOGW(DpText::TAG, "Not setting invalid hex value for %s", this->config_to_string().c_str());
            return;
         }

         if (this->matching_dp_.matches(UyatDatapointType::RAW))
         {
            to_set_dp = UyatDatapoint{
               this->matching_dp_.number,
               RawDatapointValue{parsed}
            };
         }
         if (this->matching_dp_.matches(UyatDatapointType::STRING))
         {
            to_set_dp = UyatDatapoint{
               this->matching_dp_.number,
               StringDatapointValue{std::string(parsed.begin(), parsed.end())}
            };
         }
      }
      else
      {
         if (this->matching_dp_.matches(UyatDatapointType::RAW))
         {
            to_set_dp = UyatDatapoint{
               this->matching_dp_.number,
               RawDatapointValue{std::vector<uint8_t>(value.begin(), value.end())}
            };
         }
         if (this->matching_dp_.matches(UyatDatapointType::STRING))
         {
            to_set_dp = UyatDatapoint{
               this->matching_dp_.number,
               StringDatapointValue{value}
            };
         }
      }

      if (!to_set_dp.has_value())
      {
         ESP_LOGW(DpText::TAG, "Not setting empty (after encoding) value for %s", this->config_to_string().c_str());
         return;
      }

      this->handler_->set_datapoint_value(to_set_dp.value());
   }

   DpText(DpText&&) = default;
   DpText& operator=(DpText&&) = default;

   DpText(const OnValueCallback& callback, MatchingDatapoint&& matching_dp, const TextDataEncoding data_encoding):
   callback_(callback),
   matching_dp_(std::move(matching_dp)),
   data_encoding_(data_encoding),
   handler_(nullptr)
   {}

private:

   std::string decode_(const std::string& input) const
   {
      if (input.empty())
      {
         return {};
      }

      if (this->data_encoding_ == TextDataEncoding::AS_BASE64)
      {
         const auto decoded = base64_decode(input);
         if (decoded.empty())
         {
            return {};
         }
         return std::string(decoded.begin(), decoded.end());
      }

      if (this->data_encoding_ == TextDataEncoding::AS_HEX)
      {
         return format_hex_pretty(input.data(), input.size(), 0);
      }

      return input;
   }

   OnValueCallback callback_;
   MatchingDatapoint matching_dp_;
   const TextDataEncoding data_encoding_;

   DatapointHandler* handler_;
   std::string received_value_;
   std::string set_value_;
};

}
