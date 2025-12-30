#pragma once

#include <functional>

#include "uyat_datapoint_types.h"

namespace esphome::uyat
{

struct DpBinarySensor
{
   static constexpr const char * TAG = "uyat.DpBinarySensor";

   using OnValueCallback = std::function<void(const bool)>;

   void init(const DpRegisterFunction& dp_register)
   {
      dp_register(this->matching_dp_, [this](const UyatDatapoint &datapoint) {
         ESP_LOGV(DpBinarySensor::TAG, "Processing as binary sensor %s", datapoint.to_string());

         if (auto * dp_value = std::get_if<BoolDatapointValue>(&datapoint.value))
         {
            value_ = inverted_? (!dp_value->value) : dp_value->value;
            callback_(value_.value());
         }
         else
         if (auto * dp_value = std::get_if<UIntDatapointValue>(&datapoint.value))
         {
            value_ = inverted_? (dp_value->value == 0) : (dp_value->value != 0);
            callback_(value_.value());
         }
         else
         if (auto * dp_value = std::get_if<EnumDatapointValue>(&datapoint.value))
         {
            value_ = inverted_? (dp_value->value == 0) : (dp_value->value != 0);
            callback_(value_.value());
         }
         else
         if (auto * dp_value = std::get_if<Bitmask8DatapointValue>(&datapoint.value))
         {
            if (bit_number_ >= 8)
            {
               ESP_LOGW(DpBinarySensor::TAG, "Bit number %d is too big for the 8bit bitmask!", bit_number_);
               return;
            }
            const bool bit_value = (dp_value->value & (1<<bit_number_))!=0;
            value_ = inverted_? (!bit_value) : bit_value;
            callback_(value_.value());
         }
         else
         if (auto * dp_value = std::get_if<Bitmask16DatapointValue>(&datapoint.value))
         {
            if (bit_number_ >= 16)
            {
               ESP_LOGW(DpBinarySensor::TAG, "Bit number %d is too big for the 16bit bitmask!", bit_number_);
               return;
            }
            const bool bit_value = (dp_value->value & (1<<bit_number_))!=0;
            value_ = inverted_? (!bit_value) : bit_value;
            callback_(value_.value());
         }
         else
         if (auto * dp_value = std::get_if<Bitmask32DatapointValue>(&datapoint.value))
         {
            if (bit_number_ >= 32)
            {
               ESP_LOGW(DpBinarySensor::TAG, "Bit number %d is too big for the 32bit bitmask!", bit_number_);
               return;
            }
            const bool bit_value = (dp_value->value & (1<<bit_number_))!=0;
            value_ = inverted_? (!bit_value) : bit_value;
            callback_(value_.value());
         }
         else
         {
            ESP_LOGW(DpBinarySensor::TAG, "Unhandled datapoint type %s!", datapoint.get_type_name());
            return;
         }
      });
   }

   std::optional<bool> get_last_value() const
   {
      return value_;
   }

   const std::string to_string() const
   {
      if (matching_dp_.type)
      {
         return str_sprintf("%s%s", this->inverted_? "Inverted " : "", this->matching_dp_.to_string().c_str());
      }
      else
      {
         return str_sprintf("%s Datapoint %u: BITMAP, bit %u", this->inverted_? "Inverted " : "", this->matching_dp_.number, this->bit_number_);
      }
   }

   static DpBinarySensor create_for_bool(const OnValueCallback& callback, const uint8_t dp_id, const bool inverted = false)
   {
      return DpBinarySensor(callback, MatchingDatapoint{dp_id, UyatDatapointType::BOOLEAN}, 0, inverted);
   }

   static DpBinarySensor create_for_uint(const OnValueCallback& callback, const uint8_t dp_id, const bool inverted = false)
   {
      return DpBinarySensor(callback, MatchingDatapoint{dp_id, UyatDatapointType::INTEGER}, 0, inverted);
   }

   static DpBinarySensor create_for_enum(const OnValueCallback& callback, const uint8_t dp_id, const bool inverted = false)
   {
      return DpBinarySensor(callback, MatchingDatapoint{dp_id, UyatDatapointType::ENUM}, 0, inverted);
   }

   static DpBinarySensor create_for_bitmap(const OnValueCallback& callback, const uint8_t dp_id, const uint8_t bit_number, const bool inverted = false)
   {
      // todo: set matching to any bitmask type
      return DpBinarySensor(callback, MatchingDatapoint{dp_id, {}}, bit_number, inverted);
   }

   DpBinarySensor(DpBinarySensor&&) = default;
   DpBinarySensor& operator=(DpBinarySensor&&) = default;

private:

   DpBinarySensor(const OnValueCallback& callback, MatchingDatapoint&& matching_dp, const uint8_t bit_number, const bool inverted):
   callback_(callback),
   matching_dp_(std::move(matching_dp)),
   bit_number_(bit_number),
   inverted_(inverted)
   {}

   OnValueCallback callback_;
   const MatchingDatapoint matching_dp_;
   const uint8_t bit_number_; // only matters for bitmask
   const bool inverted_;

   std::optional<bool> value_;
};

}
