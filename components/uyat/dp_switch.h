#pragma once

#include <functional>

#include "uyat_datapoint_types.h"
#include "uyat_string.hpp"

namespace esphome::uyat
{

struct DpSwitch
{
   static constexpr const char * TAG = "uyat.DpSwitch";

   using OnValueCallback = std::function<void(const bool)>;

   struct Config
   {
      MatchingDatapoint matching_dp;
      const bool inverted;

      const String to_string() const
      {
         return StringHelpers::sprintf("%s%s", this->inverted? "Inverted " : "", this->matching_dp.to_string().c_str());
      }
   };

   void init(DatapointHandler& handler)
   {
      this->handler_ = &handler;
      this->handler_->register_datapoint_listener(this->config_.matching_dp, [this](const UyatDatapoint &datapoint) {
         ESP_LOGV(DpSwitch::TAG, "%s processing as switch", datapoint.to_string().c_str());

         if (!this->config_.matching_dp.matches(datapoint.get_type()))
         {
            ESP_LOGW(DpSwitch::TAG, "Non-matching datapoint type %s!", datapoint.get_type_name());
            return;
         }

         if (auto * dp_value = std::get_if<BoolDatapointValue>(&datapoint.value))
         {
            if (!this->config_.matching_dp.allows_single_type())
            {
               this->config_.matching_dp.types = {UyatDatapointType::BOOLEAN};
               ESP_LOGI(DpSwitch::TAG, "Resolved %s", this->config_.matching_dp.to_string().c_str());
            }
            received_value_ = invert_if_needed(dp_value->value);
            callback_(received_value_.value());
         }
         else
         if (auto * dp_value = std::get_if<UIntDatapointValue>(&datapoint.value))
         {
            if (!this->config_.matching_dp.allows_single_type())
            {
               this->config_.matching_dp.types = {UyatDatapointType::INTEGER};
               ESP_LOGI(DpSwitch::TAG, "Resolved %s", this->config_.matching_dp.to_string().c_str());
            }

            received_value_ = invert_if_needed(dp_value->value != 0);
            callback_(received_value_.value());
         }
         else
         if (auto * dp_value = std::get_if<EnumDatapointValue>(&datapoint.value))
         {
            if (!this->config_.matching_dp.allows_single_type())
            {
               this->config_.matching_dp.types = {UyatDatapointType::ENUM};
               ESP_LOGI(DpSwitch::TAG, "Resolved %s", this->config_.matching_dp.to_string().c_str());
            }
            received_value_ = invert_if_needed(dp_value->value != 0);
            callback_(received_value_.value());
         }
         else
         {
            ESP_LOGW(DpSwitch::TAG, "Unhandled datapoint type %s!", datapoint.get_type_name());
            return;
         }
      });
   }

   std::optional<bool> get_last_received_value() const
   {
      return received_value_;
   }

   std::optional<bool> get_last_set_value() const
   {
      return set_value_;
   }

   const Config& get_config() const
   {
      return config_;
   }

   void set_value(const bool value, const bool force = false)
   {
      if (this->handler_ == nullptr)
      {
         ESP_LOGE(DpSwitch::TAG, "DatapointHandler not initialized for %s", this->config_.to_string().c_str());
         return;
      }

      this->set_value_ = value;
      if (!this->config_.matching_dp.allows_single_type())
      {
         ESP_LOGW(DpSwitch::TAG, "Cannot set value, datapoint type not yet known for %s", this->config_.matching_dp.to_string().c_str());
         return;
      }
      if (this->config_.matching_dp.matches(UyatDatapointType::BOOLEAN))
      {
         handler_->set_datapoint_value(UyatDatapoint{this->config_.matching_dp.number, BoolDatapointValue{invert_if_needed(value)}}, force);
      }
      else
      if (this->config_.matching_dp.matches(UyatDatapointType::INTEGER))
      {
         handler_->set_datapoint_value(UyatDatapoint{this->config_.matching_dp.number, UIntDatapointValue{invert_if_needed(value)? 0x01u : 0x00u}}, force);
      }
      else
      if (this->config_.matching_dp.matches(UyatDatapointType::ENUM))
      {
         handler_->set_datapoint_value(UyatDatapoint{this->config_.matching_dp.number, EnumDatapointValue{static_cast<decltype(EnumDatapointValue::value)>(invert_if_needed(value)? 0x01 : 0x00)}}, force);
      }
   }

   DpSwitch(DpSwitch&&) = default;
   DpSwitch& operator=(DpSwitch&&) = default;

   DpSwitch(const OnValueCallback& callback, MatchingDatapoint&& matching_dp, const bool inverted):
   config_{std::move(matching_dp), inverted},
   callback_(callback)
   {}

private:

   bool invert_if_needed(const bool logical) const
   {
      return this->config_.inverted? (!logical) : logical;
   }

   Config config_;
   OnValueCallback callback_;

   DatapointHandler* handler_{nullptr};
   std::optional<bool> received_value_;
   std::optional<bool> set_value_;
};

}
