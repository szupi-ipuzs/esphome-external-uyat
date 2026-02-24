#pragma once

#include "uyat_datapoint_types.h"
#include "dp_number.h"
#include "uyat_string.hpp"

#include <optional>
#include <cstdint>
#include <functional>

namespace esphome::uyat
{

struct DpDimmer
{
   using BrightnessChangedCallback = std::function<void(const float)>;
   static constexpr const char* TAG = "uyat.DpDimmer";

   struct Config
   {
      MatchingDatapoint matching_dp;
      const uint32_t min_value;
      const uint32_t max_value;
      const bool inverted;

      StaticString to_string() const
      {
         return StringHelpers::sprintf("%s, [%u, %u]%s", matching_dp.to_string().c_str(), min_value, max_value, inverted? " inverted":"");
      }
   };

   DpDimmer(DpDimmer&&) = default;
   DpDimmer& operator=(DpDimmer&&) = default;

   DpDimmer(BrightnessChangedCallback callback, MatchingDatapoint dimmer_dp, const uint32_t min_value, const uint32_t max_value, const bool inverted):
   config_{std::move(dimmer_dp),
           min_value,
           max_value,
           inverted
          },
   callback_(callback)
   {}

   void init(DatapointHandler& handler)
   {
      handler_ = &handler;
      handler.register_datapoint_listener(this->config_.matching_dp, [this](const UyatDatapoint &datapoint) {
         ESP_LOGV(DpNumber::TAG, "%s processing as dimmer", datapoint.to_string().c_str());
         if (!this->config_.matching_dp.matches(datapoint.get_type()))
         {
            ESP_LOGW(DpNumber::TAG, "Non-matching datapoint type %s!", datapoint.get_type_name());
            return;
         }

         if (auto * dp_value = std::get_if<UIntDatapointValue>(&datapoint.value))
         {
            if (!this->config_.matching_dp.allows_single_type())
            {
               this->config_.matching_dp.types = {UyatDatapointType::INTEGER};
               ESP_LOGI(DpNumber::TAG, "Resolved %s", this->config_.matching_dp.to_string().c_str());
            }
            last_received_value_ = mcu_value_to_percent(dp_value->value);
            if (config_.inverted)
            {
               last_received_value_ = 1.0f - *last_received_value_;
            }
            callback_(*last_received_value_);
         }
         else
         if (auto * dp_value = std::get_if<EnumDatapointValue>(&datapoint.value))
         {
            if (!this->config_.matching_dp.allows_single_type())
            {
               this->config_.matching_dp.types = {UyatDatapointType::ENUM};
               ESP_LOGI(DpNumber::TAG, "Resolved %s", this->config_.matching_dp.to_string().c_str());
            }
            last_received_value_ = mcu_value_to_percent(dp_value->value);
            if (config_.inverted)
            {
               last_received_value_ = 1.0f - *last_received_value_;
            }
            callback_(*last_received_value_);
         }
         else
         {
            ESP_LOGW(DpNumber::TAG, "Unhandled datapoint type %s!", datapoint.get_type_name());
            return;
         }
      });
   }

   void set_value(float value_percent)
   {
      if (this->handler_ == nullptr)
      {
         ESP_LOGE(DpNumber::TAG, "DatapointHandler not initialized for %s", this->config_.to_string().c_str());
         return;
      }

      last_set_value_ = value_percent;
      if (config_.inverted)
      {
         value_percent = 1.0f - value_percent;
      }
      auto value_raw = static_cast<uint32_t>(lround(value_percent * (config_.max_value - config_.min_value + 1))) + config_.min_value;
      if (value_raw > config_.max_value)
      {
         value_raw = config_.max_value;
      }
      if (value_raw < config_.min_value)
      {
         value_raw = config_.min_value;
      }

      ESP_LOGD(DpDimmer::TAG, "Setting dimmer percent %.3f (raw: %d)", *last_set_value_, value_raw);

      if (!this->config_.matching_dp.allows_single_type())
      {
         ESP_LOGW(DpNumber::TAG, "Cannot set value, datapoint type not yet known for %s", this->config_.matching_dp.to_string().c_str());
      }
      else if (this->config_.matching_dp.matches(UyatDatapointType::INTEGER))
      {
         this->handler_->set_datapoint_value(UyatDatapoint{
            this->config_.matching_dp.number,
            UIntDatapointValue{value_raw}
         });
      }
      else if (this->config_.matching_dp.matches(UyatDatapointType::ENUM))
      {
         this->handler_->set_datapoint_value(UyatDatapoint{
            this->config_.matching_dp.number,
            EnumDatapointValue{static_cast<uint8_t>(value_raw)}
         });
      }
      else
      {
         ESP_LOGW(DpNumber::TAG, "Unhandled datapoint type %s!", this->config_.matching_dp.to_string().c_str());
      }
   }

   std::optional<float> get_last_received_value() const
   {
      return last_received_value_;
   }

   std::optional<float> get_last_set_value() const
   {
      return last_set_value_;
   }

   const Config& get_config() const
   {
      return config_;
   }

private:

   float mcu_value_to_percent(const uint32_t mcu_value) const
   {
      if (mcu_value <= config_.min_value)
      {
         return 0.0f;
      }
      else
      if (mcu_value >= config_.max_value)
      {
         return 1.0f;
      }
      else
      {
         return float(mcu_value - config_.min_value)  / (config_.max_value - config_.min_value + 1);
      }
   }

   Config config_;
   BrightnessChangedCallback callback_;

   DatapointHandler* handler_{nullptr};

   std::optional<float> last_received_value_;
   std::optional<float> last_set_value_;
};

}
