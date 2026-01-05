#pragma once

#include <functional>

#include "uyat_datapoint_types.h"

namespace esphome::uyat
{

struct DpBinarySensor
{
   static constexpr const char * TAG = "uyat.DpBinarySensor";

   using OnValueCallback = std::function<void(const bool)>;

   void init(DatapointHandler& handler)
   {
      handler.register_datapoint_listener(this->matching_dp_, [this](const UyatDatapoint &datapoint) {
         ESP_LOGV(DpBinarySensor::TAG, "%s processing as binary sensor", datapoint.to_string().c_str());

         if (!matching_dp_.matches(datapoint.get_type()))
         {
            ESP_LOGW(DpBinarySensor::TAG, "Non-matching datapoint type %s!", datapoint.get_type_name());
            return;
         }

         if (auto * dp_value = std::get_if<BoolDatapointValue>(&datapoint.value))
         {
            if (!this->matching_dp_.allows_single_type())
            {
               this->matching_dp_.types = {UyatDatapointType::BOOLEAN};
               ESP_LOGI(DpBinarySensor::TAG, "Resolved %s", this->matching_dp_.to_string().c_str());
            }
            value_ = inverted_? (!dp_value->value) : dp_value->value;
            callback_(value_.value());
         }
         else
         if (auto * dp_value = std::get_if<UIntDatapointValue>(&datapoint.value))
         {
            if (!this->matching_dp_.allows_single_type())
            {
               this->matching_dp_.types = {UyatDatapointType::INTEGER};
               ESP_LOGI(DpBinarySensor::TAG, "Resolved %s", this->matching_dp_.to_string().c_str());
            }
            value_ = inverted_? (dp_value->value == 0) : (dp_value->value != 0);
            callback_(value_.value());
         }
         else
         if (auto * dp_value = std::get_if<EnumDatapointValue>(&datapoint.value))
         {
            if (!this->matching_dp_.allows_single_type())
            {
               this->matching_dp_.types = {UyatDatapointType::ENUM};
               ESP_LOGI(DpBinarySensor::TAG, "Resolved %s", this->matching_dp_.to_string().c_str());
            }
            value_ = inverted_? (dp_value->value == 0) : (dp_value->value != 0);
            callback_(value_.value());
         }
         else
         if (auto * dp_value = std::get_if<BitmapDatapointValue>(&datapoint.value))
         {
            if (!this->matching_dp_.allows_single_type())
            {
               this->matching_dp_.types = {UyatDatapointType::BITMAP};
               ESP_LOGI(DpBinarySensor::TAG, "Resolved %s", this->matching_dp_.to_string().c_str());
            }

            if (bit_number_ >= 32)
            {
               value_ = false;
            }
            else
            {
               const bool bit_value = (dp_value->value & (1<<bit_number_))!=0;
               value_ = inverted_? (!bit_value) : bit_value;
            }

            callback_(value_.value());
         }
      });
   }

   std::optional<bool> get_last_value() const
   {
      return value_;
   }

   std::string config_to_string() const
   {
      return str_sprintf("%s%s%s",
         this->inverted_? "Inverted " : "",
         this->matching_dp_.to_string().c_str(),
         this->matching_dp_.matches(UyatDatapointType::BITMAP)? str_sprintf(", bit %u", this->bit_number_).c_str() : ""  );
   }

   DpBinarySensor(DpBinarySensor&&) = default;
   DpBinarySensor& operator=(DpBinarySensor&&) = default;

   DpBinarySensor(const OnValueCallback& callback, MatchingDatapoint&& matching_dp, const uint8_t bit_number, const bool inverted):
   callback_(callback),
   matching_dp_(std::move(matching_dp)),
   bit_number_(bit_number),
   inverted_(inverted)
   {}

private:

   OnValueCallback callback_;
   MatchingDatapoint matching_dp_;
   const uint8_t bit_number_; // only matters for bitmap
   const bool inverted_;

   std::optional<bool> value_;
};

}
