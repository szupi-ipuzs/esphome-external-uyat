#pragma once

#include <functional>

#include "uyat_datapoint_types.h"
#include "uyat_string.hpp"

namespace esphome::uyat
{

struct DpBinarySensor
{
   static constexpr const char * TAG = "uyat.DpBinarySensor";

   using OnValueCallback = std::function<void(const bool)>;

   struct Config
   {
      MatchingDatapoint matching_dp;
      const std::optional<uint8_t> bit_number;
      const bool inverted;

      String to_string() const
      {
         return StringHelpers::sprintf("%s%s%s",
            inverted? "Inverted " : "",
            matching_dp.to_string().c_str(),
            bit_number? StringHelpers::sprintf(", bit %u", bit_number.value()).c_str() : ", whole"  );
      }
   };

   void init(DatapointHandler& handler)
   {
      handler.register_datapoint_listener(this->config_.matching_dp, [this](const UyatDatapoint &datapoint) {
         ESP_LOGV(DpBinarySensor::TAG, "%s processing as binary sensor", datapoint.to_string().c_str());

         if (!this->config_.matching_dp.matches(datapoint.get_type()))
         {
            ESP_LOGW(DpBinarySensor::TAG, "Non-matching datapoint type %s!", datapoint.get_type_name());
            return;
         }

         if (auto * dp_value = std::get_if<BoolDatapointValue>(&datapoint.value))
         {
            if (!this->config_.matching_dp.allows_single_type())
            {
               this->config_.matching_dp.types = {UyatDatapointType::BOOLEAN};
               ESP_LOGI(DpBinarySensor::TAG, "Resolved %s", this->config_.matching_dp.to_string().c_str());
            }
            this->value_ = apply_filters_(dp_value->value);
            callback_(value_.value());
         }
         else
         if (auto * dp_value = std::get_if<UIntDatapointValue>(&datapoint.value))
         {
            if (!this->config_.matching_dp.allows_single_type())
            {
               this->config_.matching_dp.types = {UyatDatapointType::INTEGER};
               ESP_LOGI(DpBinarySensor::TAG, "Resolved %s", this->config_.matching_dp.to_string().c_str());
            }
            this->value_ = apply_filters_(dp_value->value);
            callback_(value_.value());
         }
         else
         if (auto * dp_value = std::get_if<EnumDatapointValue>(&datapoint.value))
         {
            if (!this->config_.matching_dp.allows_single_type())
            {
               this->config_.matching_dp.types = {UyatDatapointType::ENUM};
               ESP_LOGI(DpBinarySensor::TAG, "Resolved %s", this->config_.matching_dp.to_string().c_str());
            }
            this->value_ = apply_filters_(dp_value->value);
            callback_(value_.value());
         }
         else
         if (auto * dp_value = std::get_if<BitmapDatapointValue>(&datapoint.value))
         {
            if (!this->config_.matching_dp.allows_single_type())
            {
               this->config_.matching_dp.types = {UyatDatapointType::BITMAP};
               ESP_LOGI(DpBinarySensor::TAG, "Resolved %s", this->config_.matching_dp.to_string().c_str());
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

   const Config& get_config() const
   {
      return config_;
   }

   DpBinarySensor(DpBinarySensor&&) = default;
   DpBinarySensor& operator=(DpBinarySensor&&) = default;

   DpBinarySensor(const OnValueCallback& callback, MatchingDatapoint&& matching_dp, const std::optional<uint8_t> bit_number, const bool inverted):
   config_{std::move(matching_dp), bit_number, inverted},
   callback_(callback)
   {}

private:

   bool apply_filters_(const uint32_t raw_value) const
   {
      bool result;
      if (this->config_.bit_number)
      {
         if (this->config_.bit_number.value() >= 32)
         {
            return false;
         }
         result = (raw_value & (1<<this->config_.bit_number.value()))!=0;
      }
      else
      {
         result = raw_value != 0u;
      }
      return this->config_.inverted? (!result) : result;
   }

   Config config_;
   OnValueCallback callback_;

   std::optional<bool> value_;
};

}
