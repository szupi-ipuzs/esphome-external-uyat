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
            this->value_ = apply_filters_(dp_value->value);
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
            this->value_ = apply_filters_(dp_value->value);
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
            this->value_ = apply_filters_(dp_value->value);
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

            this->value_ = apply_filters_(dp_value->value);
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

   std::string config_to_string() const
   {
      return str_sprintf("%s%s%s",
         this->inverted_? "Inverted " : "",
         this->matching_dp_.to_string().c_str(),
         this->bit_number_? str_sprintf(", bit %u", this->bit_number_.value()).c_str() : ", whole"  );
   }

   DpBinarySensor(DpBinarySensor&&) = default;
   DpBinarySensor& operator=(DpBinarySensor&&) = default;

   DpBinarySensor(const OnValueCallback& callback, MatchingDatapoint&& matching_dp, const std::optional<uint8_t> bit_number, const bool inverted):
   callback_(callback),
   matching_dp_(std::move(matching_dp)),
   bit_number_(bit_number),
   inverted_(inverted)
   {}

private:

   bool apply_filters_(const uint32_t raw_value) const
   {
      bool result;
      if (this->bit_number_)
      {
         if (this->bit_number_.value() >= 32)
         {
            return false;
         }
         result = (raw_value & (1<<this->bit_number_.value()))!=0;
      }
      else
      {
         result = raw_value != 0u;
      }
      return this->inverted_? (!result) : result;
   }

   OnValueCallback callback_;
   MatchingDatapoint matching_dp_;
   const std::optional<uint8_t> bit_number_;
   const bool inverted_;

   std::optional<bool> value_;
};

}
