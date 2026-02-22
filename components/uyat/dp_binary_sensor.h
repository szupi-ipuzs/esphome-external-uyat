#pragma once

#include <functional>

#include "uyat_datapoint_types.h"

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

      void to_string(char* buffer, size_t size) const
      {
         char match_temp[UYAT_LOG_BUFFER_SIZE];
         matching_dp.to_string(match_temp, sizeof(match_temp));

         int written = snprintf(buffer, size, "%s%s",
                                inverted? "Inverted " : "",
                                match_temp);
         size_t pos = (written > 0) ? static_cast<size_t>(written) : 0u;
         if (pos >= size)
         {
            return;
         }

         if (bit_number) {
            snprintf(buffer + pos, size - pos, ", bit %u", bit_number.value());
         } else {
            snprintf(buffer + pos, size - pos, ", whole");
         }
      }
   };

   void init(DatapointHandler& handler)
   {
      handler.register_datapoint_listener(this->config_.matching_dp, [this](const UyatDatapoint &datapoint) {
         char temp_str[UYAT_PRETTY_HEX_BUFFER_SIZE];
         datapoint.to_string(temp_str, sizeof(temp_str));
         ESP_LOGV(DpBinarySensor::TAG, "%s processing as binary sensor", temp_str);

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
               this->config_.matching_dp.to_string(temp_str, sizeof(temp_str));
               ESP_LOGI(DpBinarySensor::TAG, "Resolved %s", temp_str);
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
               this->config_.matching_dp.to_string(temp_str, sizeof(temp_str));
               ESP_LOGI(DpBinarySensor::TAG, "Resolved %s", temp_str);
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
               this->config_.matching_dp.to_string(temp_str, sizeof(temp_str));
               ESP_LOGI(DpBinarySensor::TAG, "Resolved %s", temp_str);
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
               this->config_.matching_dp.to_string(temp_str, sizeof(temp_str));
               ESP_LOGI(DpBinarySensor::TAG, "Resolved %s", temp_str);
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
