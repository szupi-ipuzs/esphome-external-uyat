#pragma once

#include "esphome/core/helpers.h"
#include "uyat_datapoint_types.h"

#include <functional>

namespace esphome::uyat
{

enum class UyatColorType {
  RGB,
  HSV,
  RGBHSV,
};

struct DpColor
{
   static constexpr const char * TAG = "uyat.DpColor";

   static constexpr const char* color_type_to_string(const UyatColorType color_type)
   {
      if (color_type == UyatColorType::RGB)
      {
         return "RGB";
      }

      if (color_type == UyatColorType::HSV)
      {
         return "HSV";
      }

      if (color_type == UyatColorType::RGBHSV)
      {
         return "RGBHSV";
      }

      return "UNKNOWN";
   }

   struct Value
   {
      float r;
      float g;
      float b;
   };

   struct Config
   {
      MatchingDatapoint matching_dp;
      const UyatColorType color_type;

      void to_string(char* buffer, size_t size) const
      {
         char temp_dp[UYAT_LOG_BUFFER_SIZE];
         matching_dp.to_string(temp_dp, sizeof(temp_dp));
         snprintf(buffer, size, "%s, color_type: %s",
                  temp_dp, DpColor::color_type_to_string(color_type));
      }
   };

   using Callback = std::function<void(const Value&)>;

   DpColor(Callback callback, MatchingDatapoint color_dp, const UyatColorType color_type):
   config_{std::move(color_dp), color_type},
   callback_(callback)
   {}

   void init(DatapointHandler& handler)
   {
      handler_ = &handler;
      handler.register_datapoint_listener(this->config_.matching_dp, [this](const UyatDatapoint &datapoint) {
         char datapoint_str[UYAT_PRETTY_HEX_BUFFER_SIZE];
         datapoint.to_string(datapoint_str, sizeof(datapoint_str));
         ESP_LOGV(DpColor::TAG, "%s processing as color", datapoint_str);

         if (!this->config_.matching_dp.matches(datapoint.get_type()))
         {
            ESP_LOGW(DpColor::TAG, "Non-matching datapoint type %s!", datapoint.get_type_name());
            return;
         }

         if (auto * dp_value = std::get_if<StringDatapointValue>(&datapoint.value))
         {
            if (!this->config_.matching_dp.allows_single_type())
            {
               this->config_.matching_dp.types = {UyatDatapointType::STRING};
               char match_str[UYAT_LOG_BUFFER_SIZE];
               this->config_.matching_dp.to_string(match_str, sizeof(match_str));
               ESP_LOGI(DpColor::TAG, "Resolved %s", match_str);
            }
            auto new_value = this->decode_(dp_value->value);
            if (new_value)
            {
               this->last_received_value_ = new_value;
               callback_(*new_value);
            }
            else
            {
               ESP_LOGW(DpColor::TAG, "Failed to decode color %s!", datapoint_str);
            }
         }
         else
         {
            ESP_LOGW(DpColor::TAG, "Unhandled datapoint type %s!", datapoint.get_type_name());
            return;
         }
      });
   }

   void set_value(const Value& v)
   {
      this->last_set_value_ = v;
      if (this->handler_ == nullptr)
      {
         char config_str[UYAT_LOG_BUFFER_SIZE];
         this->config_.to_string(config_str, sizeof(config_str));
         ESP_LOGE(DpColor::TAG, "DatapointHandler not initialized for %s", config_str);
         return;
      }

      if (this->config_.color_type == UyatColorType::RGB)
      {
         this->handler_->set_datapoint_value(UyatDatapoint{
                                       this->config_.matching_dp.number,
                                       StringDatapointValue{to_raw_rgb(v)}
                                       });
      }
      else
      if (this->config_.color_type == UyatColorType::HSV)
      {
         this->handler_->set_datapoint_value(UyatDatapoint{
                                       this->config_.matching_dp.number,
                                       StringDatapointValue{to_raw_hsv(v)}
                                       });
      }
      else
      if (this->config_.color_type == UyatColorType::RGBHSV)
      {
         this->handler_->set_datapoint_value(UyatDatapoint{
                                       this->config_.matching_dp.number,
                                       StringDatapointValue{to_raw_rgbhsv(v)}
                                       });
      }
   }

   std::optional<Value> get_last_set_value() const
   {
      return last_set_value_;
   }

   std::optional<Value> get_last_received_value() const
   {
      return last_received_value_;
   }

   const Config& get_config() const
   {
      return config_;
   }

private:

   std::string to_raw_rgb(const Value& v) const
   {
      std::string buffer(6u, '0');
      sprintf(buffer.data(), "%02X%02X%02X", int(v.r * 255), int(v.g * 255), int(v.b * 255));
      return buffer;
   }

   std::string to_raw_hsv(const Value& v) const
   {
      std::string buffer(12u, '0');
      int hue;
      float saturation, value;
      rgb_to_hsv(v.r, v.g, v.b, hue, saturation, value);
      sprintf(buffer.data(), "%04X%04X%04X", hue, int(saturation * 1000), int(value * 1000));
      return buffer;
   }

   std::string to_raw_rgbhsv(const Value& v) const
   {
      std::string buffer(14u, '0');
      int hue;
      float saturation, value;
      rgb_to_hsv(v.r, v.g, v.b, hue, saturation, value);
      sprintf(buffer.data(), "%02X%02X%02X%04X%02X%02X", int(v.r * 255), int(v.g * 255), int(v.b * 255), hue,
               int(saturation * 255), int(value * 255));
      return buffer;
   }

   std::optional<Value> decode_(const std::string& raw_value) const
   {
      if (this->config_.color_type == UyatColorType::RGB)
      {
         return decode_as_rgb_(raw_value);
      }
      else
      if (this->config_.color_type == UyatColorType::HSV)
      {
         return decode_as_hsv_(raw_value);
      }
      else
      if (this->config_.color_type == UyatColorType::RGBHSV)
      {
         return decode_as_rgbhsv_(raw_value);
      }

      return std::nullopt;
   }

   std::optional<Value> decode_as_rgb_(const std::string& raw_value) const
   {
      const auto rgb = parse_hex<uint32_t>(raw_value.substr(0, 6));
      if (!rgb.has_value())
      {
         return std::nullopt;
      }

      return Value{(*rgb >> 16) / 255.0f,
                   ((*rgb >> 8) & 0xff) / 255.0f,
                   (*rgb & 0xff) / 255.0f};
   }

   std::optional<Value> decode_as_hsv_(const std::string& raw_value) const
   {
      const auto hue = parse_hex<uint16_t>(raw_value.substr(0, 4));
      const auto saturation = parse_hex<uint16_t>(raw_value.substr(4, 4));
      const auto value = parse_hex<uint16_t>(raw_value.substr(8, 4));
      if (!hue.has_value() || !saturation.has_value() || !value.has_value())
      {
         return std::nullopt;
      }

      Value result{};
      hsv_to_rgb(*hue, float(*saturation) / 1000, float(*value) / 1000, result.r, result.g, result.b);
      return result;
   }

   std::optional<Value> decode_as_rgbhsv_(const std::string& raw_value) const
   {
      return decode_as_rgb_(raw_value);
   }

   Config config_;
   Callback callback_;

   DatapointHandler* handler_{nullptr};
   std::optional<Value> last_received_value_;
   std::optional<Value> last_set_value_;
};

}
