#pragma once

#include <functional>

#include "uyat_datapoint_types.h"

namespace esphome::uyat
{

struct DpSwitch
{
   static constexpr const char * TAG = "uyat.DpSwitch";

   using OnValueCallback = std::function<void(const bool)>;

   void init(DatapointHandler& handler)
   {
      handler_ = &handler;
      this->handler_->register_datapoint_listener(this->matching_dp_, [this](const UyatDatapoint &datapoint) {
         ESP_LOGV(DpSwitch::TAG, "%s processing as switch", datapoint.to_string().c_str());

         if (!matching_dp_.matches(datapoint.get_type()))
         {
            ESP_LOGW(DpSwitch::TAG, "Non-matching datapoint type %s!", datapoint.get_type_name());
            return;
         }

         if (auto * dp_value = std::get_if<BoolDatapointValue>(&datapoint.value))
         {
            if (!this->matching_dp_.allows_single_type())
            {
               this->matching_dp_.types = {UyatDatapointType::BOOLEAN};
               ESP_LOGI(DpSwitch::TAG, "Resolved %s", this->matching_dp_.to_string().c_str());
            }
            received_value_ = invert_if_needed(dp_value->value);
            callback_(received_value_.value());
         }
         else
         if (auto * dp_value = std::get_if<UIntDatapointValue>(&datapoint.value))
         {
            if (!this->matching_dp_.allows_single_type())
            {
               this->matching_dp_.types = {UyatDatapointType::INTEGER};
               ESP_LOGI(DpSwitch::TAG, "Resolved %s", this->matching_dp_.to_string().c_str());
            }

            received_value_ = invert_if_needed(dp_value->value != 0);
            callback_(received_value_.value());
         }
         else
         if (auto * dp_value = std::get_if<EnumDatapointValue>(&datapoint.value))
         {
            if (!this->matching_dp_.allows_single_type())
            {
               this->matching_dp_.types = {UyatDatapointType::ENUM};
               ESP_LOGI(DpSwitch::TAG, "Resolved %s", this->matching_dp_.to_string().c_str());
            }
            received_value_ = invert_if_needed(dp_value->value != 0);
            callback_(received_value_.value());
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

   const std::string config_to_string() const
   {
      return str_sprintf("%s%s", this->inverted_? "Inverted " : "", this->matching_dp_.to_string().c_str());
   }

   void set_value(const bool value, const bool force = false)
   {
      if (this->handler_ == nullptr)
      {
         ESP_LOGE(DpSwitch::TAG, "DatapointHandler not initialized for %s", this->config_to_string().c_str());
         return;
      }

      this->set_value_ = value;
      if (!this->matching_dp_.allows_single_type())
      {
         ESP_LOGW(DpSwitch::TAG, "Cannot set value, datapoint type not yet known for %s", this->matching_dp_.to_string().c_str());
         return;
      }
      if (this->matching_dp_.matches(UyatDatapointType::BOOLEAN))
      {
         handler_->set_datapoint_value(UyatDatapoint{this->matching_dp_.number, BoolDatapointValue{invert_if_needed(value)}}, force);
      }
      else
      if (this->matching_dp_.matches(UyatDatapointType::INTEGER))
      {
         handler_->set_datapoint_value(UyatDatapoint{this->matching_dp_.number, UIntDatapointValue{invert_if_needed(value)? 0x01u : 0x00u}}, force);
      }
      else
      if (this->matching_dp_.matches(UyatDatapointType::ENUM))
      {
         handler_->set_datapoint_value(UyatDatapoint{this->matching_dp_.number, EnumDatapointValue{invert_if_needed(value)? 0x01 : 0x00}}, force);
      }
   }

   DpSwitch(DpSwitch&&) = default;
   DpSwitch& operator=(DpSwitch&&) = default;

   DpSwitch(const OnValueCallback& callback, MatchingDatapoint&& matching_dp, const bool inverted):
   callback_(callback),
   matching_dp_(std::move(matching_dp)),
   inverted_(inverted)
   {}

private:

   bool invert_if_needed(const bool logical) const
   {
      return this->inverted_? (!logical) : logical;
   }

   OnValueCallback callback_;
   MatchingDatapoint matching_dp_;
   const bool inverted_;

   DatapointHandler* handler_;
   std::optional<bool> received_value_;
   std::optional<bool> set_value_;
};

}
