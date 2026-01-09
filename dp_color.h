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
   struct Value
   {
      float r;
      float g;
      float b;
   };

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

   using Callback = std::function<void(const Value&)>;
   DpColor(Callback callback, MatchingDatapoint color_dp, const UyatColorType color_type):
   callback_(callback),
   matching_dp_(std::move(color_dp)),
   color_type_(color_type)
   {}

   void init(DatapointHandler& handler)
   {
      handler_ = &handler;
      handler.register_datapoint_listener(this->matching_dp_, [this](const UyatDatapoint &datapoint) {
         ESP_LOGV(DpColor::TAG, "%s processing as color", datapoint.to_string().c_str());

         if (!matching_dp_.matches(datapoint.get_type()))
         {
            ESP_LOGW(DpColor::TAG, "Non-matching datapoint type %s!", datapoint.get_type_name());
            return;
         }

         if (auto * dp_value = std::get_if<StringDatapointValue>(&datapoint.value))
         {
            if (!this->matching_dp_.allows_single_type())
            {
               this->matching_dp_.types = {UyatDatapointType::STRING};
               ESP_LOGI(DpColor::TAG, "Resolved %s", this->matching_dp_.to_string().c_str());
            }
            auto new_value = this->decode_(dp_value->value);
            if (new_value)
            {
               this->last_received_value_ = new_value;
               callback_(*new_value);
            }
            else
            {
               ESP_LOGW(DpColor::TAG, "Failed to decode color %s!", datapoint.to_string().c_str());
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
         ESP_LOGE(DpColor::TAG, "DatapointHandler not initialized for %s", this->config_to_string().c_str());
         return;
      }

      if (this->color_type_ == UyatColorType::RGB)
      {
         this->handler_->set_datapoint_value(UyatDatapoint{
                                       this->matching_dp_.number,
                                       StringDatapointValue{to_raw_rgb(v)}
                                       });
      }
      else
      if (this->color_type_ == UyatColorType::HSV)
      {
         this->handler_->set_datapoint_value(UyatDatapoint{
                                       this->matching_dp_.number,
                                       StringDatapointValue{to_raw_hsv(v)}
                                       });
      }
      else
      if (this->color_type_ == UyatColorType::RGBHSV)
      {
         this->handler_->set_datapoint_value(UyatDatapoint{
                                       this->matching_dp_.number,
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

   std::string config_to_string() const
   {
      return str_sprintf("%s, color_type: %s", this->matching_dp_.to_string().c_str(), color_type_to_string(color_type_));
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
      if (this->color_type_ == UyatColorType::RGB)
      {
         return decode_as_rgb_(raw_value);
      }
      else
      if (this->color_type_ == UyatColorType::HSV)
      {
         return decode_as_hsv_(raw_value);
      }
      else
      if (this->color_type_ == UyatColorType::RGBHSV)
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

   Callback callback_;
   MatchingDatapoint matching_dp_;
   UyatColorType color_type_;

   DatapointHandler* handler_{nullptr};
   std::optional<Value> last_received_value_;
   std::optional<Value> last_set_value_;
};

}
