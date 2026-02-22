#pragma once

#include <functional>

#include "uyat_datapoint_types.h"

namespace esphome::uyat
{

struct DpNumber
{
   static constexpr const char * TAG = "uyat.DpNumber";

   using OnValueCallback = std::function<void(const float)>;

   struct Config
   {
      MatchingDatapoint matching_dp;
      const float offset;
      const float multiplier;

      void to_string(char* buffer, size_t size) const
      {
         char temp_dp[UYAT_LOG_BUFFER_SIZE];
         matching_dp.to_string(temp_dp, sizeof(temp_dp));
         snprintf(buffer, size, "%s, offset=%.2f, multiplier=%.2f",
                  temp_dp, offset, multiplier);
      }
   };

   void init(DatapointHandler& handler)
   {
      handler_ = &handler;
      handler.register_datapoint_listener(this->config_.matching_dp, [this](const UyatDatapoint &datapoint) {
         char datapoint_str[UYAT_PRETTY_HEX_BUFFER_SIZE];
         datapoint.to_string(datapoint_str, sizeof(datapoint_str));
         ESP_LOGV(DpNumber::TAG, "%s processing as number", datapoint_str);

         if (!this->config_.matching_dp.matches(datapoint.get_type()))
         {
            ESP_LOGW(DpNumber::TAG, "Non-matching datapoint type %s!", datapoint.get_type_name());
            return;
         }

         if (auto * dp_value = std::get_if<BoolDatapointValue>(&datapoint.value))
         {
            if (!this->config_.matching_dp.allows_single_type())
            {
               this->config_.matching_dp.types = {UyatDatapointType::BOOLEAN};
               char match_str[UYAT_LOG_BUFFER_SIZE];
               this->config_.matching_dp.to_string(match_str, sizeof(match_str));
               ESP_LOGI(DpNumber::TAG, "Resolved %s", match_str);
            }
            this->last_received_value_ = calculate_logical_value(dp_value->value);
            callback_(last_received_value_.value());
         }
         else
         if (auto * dp_value = std::get_if<UIntDatapointValue>(&datapoint.value))
         {
            if (!this->config_.matching_dp.allows_single_type())
            {
               this->config_.matching_dp.types = {UyatDatapointType::INTEGER};
               char match_str[UYAT_LOG_BUFFER_SIZE];
               this->config_.matching_dp.to_string(match_str, sizeof(match_str));
               ESP_LOGI(DpNumber::TAG, "Resolved %s", match_str);
            }
            this->last_received_value_ = calculate_logical_value(dp_value->value);
            callback_(last_received_value_.value());
         }
         else
         if (auto * dp_value = std::get_if<EnumDatapointValue>(&datapoint.value))
         {
            if (!this->config_.matching_dp.allows_single_type())
            {
               this->config_.matching_dp.types = {UyatDatapointType::ENUM};
               char match_str[UYAT_LOG_BUFFER_SIZE];
               this->config_.matching_dp.to_string(match_str, sizeof(match_str));
               ESP_LOGI(DpNumber::TAG, "Resolved %s", match_str);
            }
            this->last_received_value_ = calculate_logical_value(dp_value->value);
            callback_(last_received_value_.value());
         }
         else
         if (auto * dp_value = std::get_if<BitmapDatapointValue>(&datapoint.value))
         {
            if (!this->config_.matching_dp.allows_single_type())
            {
               this->config_.matching_dp.types = {UyatDatapointType::BITMAP};
               char match_str[UYAT_LOG_BUFFER_SIZE];
               this->config_.matching_dp.to_string(match_str, sizeof(match_str));
               ESP_LOGI(DpNumber::TAG, "Resolved %s", match_str);
            }
            this->last_received_value_ = calculate_logical_value(dp_value->value);
            callback_(last_received_value_.value());
         }
         else
         {
            ESP_LOGW(DpNumber::TAG, "Unhandled datapoint type %s!", datapoint.get_type_name());
            return;
         }
      });
   }

   std::optional<float> get_last_received_value() const
   {
      return this->last_received_value_;
   }

   std::optional<float> get_last_set_value() const
   {
      return this->last_set_value_;
   }

   const Config& get_config() const
   {
      return config_;
   }

   void set_value(const float value, const bool forced = false)
   {
      char config_str[UYAT_LOG_BUFFER_SIZE];
      
      if (this->handler_ == nullptr)
      {
         this->config_.to_string(config_str, sizeof(config_str));
         ESP_LOGE(DpNumber::TAG, "DatapointHandler not initialized for %s", config_str);
         return;
      }

      this->get_config().to_string(config_str, sizeof(config_str));
      ESP_LOGV(DpNumber::TAG, "Setting value to %.3f for %s", value, config_str);
      this->last_set_value_ = value;
      uint32_t raw_value = static_cast<uint32_t>(lround((value - this->config_.offset)* this->config_.multiplier));
      if (!this->config_.matching_dp.allows_single_type())
      {
         this->config_.matching_dp.to_string(config_str, sizeof(config_str));
         ESP_LOGW(DpNumber::TAG, "Cannot set value, datapoint type not yet known for %s", config_str);
      }
      else
      if (this->config_.matching_dp.matches(UyatDatapointType::BITMAP))
      {
         this->handler_->set_datapoint_value(UyatDatapoint{
            this->config_.matching_dp.number,
            BitmapDatapointValue{raw_value}
         }, forced);
      }
      else
      if (this->config_.matching_dp.matches(UyatDatapointType::BOOLEAN))
      {
         this->handler_->set_datapoint_value(UyatDatapoint{
            this->config_.matching_dp.number,
            BoolDatapointValue{raw_value != 0u}
         }, forced);
      }
      else if (this->config_.matching_dp.matches(UyatDatapointType::INTEGER))
      {
         this->handler_->set_datapoint_value(UyatDatapoint{
            this->config_.matching_dp.number,
            UIntDatapointValue{raw_value}
         }, forced);
      }
      else if (this->config_.matching_dp.matches(UyatDatapointType::ENUM))
      {
         this->handler_->set_datapoint_value(UyatDatapoint{
            this->config_.matching_dp.number,
            EnumDatapointValue{static_cast<uint8_t>(raw_value)}
         }, forced);
      }
      else
      {
         this->config_.matching_dp.to_string(config_str, sizeof(config_str));
         ESP_LOGW(DpNumber::TAG, "Unhandled datapoint type %s!", config_str);
      }
   }

   DpNumber(DpNumber&&) = default;
   DpNumber& operator=(DpNumber&&) = default;

   DpNumber(const OnValueCallback& callback, MatchingDatapoint&& matching_dp, const float offset, const float multiplier):
   config_{std::move(matching_dp), offset, multiplier},
   callback_(callback)
   {}

private:

   float calculate_logical_value(const uint32_t value) const
   {
      return (float(value) / this->config_.multiplier) + this->config_.offset;
   }

   Config config_;
   OnValueCallback callback_;

   DatapointHandler* handler_{nullptr};

   std::optional<float> last_received_value_;
   std::optional<float> last_set_value_;
};

}
