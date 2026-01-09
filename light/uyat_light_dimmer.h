#pragma once

#include "../uyat_datapoint_types.h"
#include "../dp_number.h"

#include <optional>
#include <cstdint>
#include <functional>

namespace esphome::uyat
{

struct UyatLightDimmer
{
   using BrightnessChangedCallback = std::function<void(const float)>;
   static constexpr const char* TAG = "uyat.UyatLightDimmer";

   UyatLightDimmer(BrightnessChangedCallback callback, MatchingDatapoint dimmer_dp, const uint32_t min_value, const uint32_t max_value, std::optional<MatchingDatapoint> min_value_dp = std::nullopt):
   callback_(callback),
   dp_dimmer_([this](const float value){ on_dimmer_value(value); },
                        std::move(dimmer_dp),
                        0, 1.0f),
   min_value_(min_value),
   max_value_(max_value)
   {
      if (min_value_dp.has_value())
      {
         dp_min_value_.emplace( std::move(DpNumber(
                        [this](const float){},  // ignore
                        std::move(*min_value_dp),
                        0, 1.0f
         )));
      }
   }

   void init(DatapointHandler& handler)
   {
      dp_dimmer_.init(handler);
      if (dp_min_value_)
      {
         dp_min_value_->init(handler);
         dp_min_value_->set_value(min_value_);
      }
   }

   void dump_config() const
   {
      ESP_LOGCONFIG(UyatLightDimmer::TAG, "   Dimmer is %s", dp_dimmer_.config_to_string().c_str());
      ESP_LOGCONFIG(UyatLightDimmer::TAG, "   Min value: %u", min_value_);
      ESP_LOGCONFIG(UyatLightDimmer::TAG, "   Max value: %u", max_value_);
      if (dp_min_value_.has_value())
      {
         ESP_LOGCONFIG(TAG, "   Has min_value_datapoint: %s", dp_min_value_->config_to_string().c_str());
      }
   }

   void set_brightness(const float value_percent)
   {
      auto brightness_int = static_cast<uint32_t>(value_percent * (max_value_ - min_value_ + 1)) + min_value_;
      if (brightness_int > max_value_)
      {
         brightness_int = max_value_;
      }
      if (brightness_int < min_value_)
      {
         brightness_int = min_value_;
      }

      ESP_LOGD(UyatLightDimmer::TAG, "Setting dimmer, brightness %.3f, brightness_int %d", value_percent, brightness_int);
      dp_dimmer_.set_value(brightness_int);
   }

private:

   void on_dimmer_value(const float value)
   {
      ESP_LOGV(UyatLightDimmer::TAG, "MCU reported dimmer %s is: %.4f", get_object_id().c_str(), value);

      callback_(mcu_value_to_brightness(value));
   }

   float mcu_value_to_brightness(const float mcu_value) const
   {
      if (mcu_value <= min_value_)
      {
         return 0.0f;
      }
      else
      if (mcu_value >= max_value_)
      {
         return 1.0f;
      }
      else
      {
         return (mcu_value - min_value_)  / (max_value_ - min_value_ + 1);
      }
   }

   BrightnessChangedCallback callback_;
   DpNumber dp_dimmer_;
   uint32_t min_value_{0};
   uint32_t max_value_{255};
   std::optional<DpNumber> dp_min_value_{std::nullopt};
};

}
