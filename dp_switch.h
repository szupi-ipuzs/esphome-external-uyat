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
         ESP_LOGV(DpSwitch::TAG, "%s processing as switch", datapoint.to_string());

         if (auto * dp_value = std::get_if<BoolDatapointValue>(&datapoint.value))
         {
            received_value_ = invert_if_needed(dp_value->value);
            callback_(received_value_.value());
         }
         else
         if (auto * dp_value = std::get_if<UIntDatapointValue>(&datapoint.value))
         {
            received_value_ = invert_if_needed(dp_value->value != 0);
            callback_(received_value_.value());
         }
         else
         if (auto * dp_value = std::get_if<EnumDatapointValue>(&datapoint.value))
         {
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

   const std::string to_string() const
   {
      return str_sprintf("%s%s", this->inverted_? "Inverted " : "", this->matching_dp_.to_string().c_str());
   }

   void set_value(const bool value, const bool force = false)
   {
      assert(handler_);

      this->set_value_ = value;
      if (this->matching_dp_.type == UyatDatapointType::BOOLEAN)
      {
         handler_->set_datapoint_value(UyatDatapoint{this->matching_dp_.number, BoolDatapointValue{invert_if_needed(value)}}, force);
      }
      else
      if (this->matching_dp_.type == UyatDatapointType::INTEGER)
      {
         handler_->set_datapoint_value(UyatDatapoint{this->matching_dp_.number, UIntDatapointValue{invert_if_needed(value)? 0x01u : 0x00u}}, force);
      }
      else
      if (this->matching_dp_.type == UyatDatapointType::ENUM)
      {
         handler_->set_datapoint_value(UyatDatapoint{this->matching_dp_.number, UIntDatapointValue{invert_if_needed(value)? 0x01u : 0x00u}}, force);
      }
   }

   static DpSwitch create_for_bool(const OnValueCallback& callback, const uint8_t dp_id, const bool inverted = false)
   {
      return DpSwitch(callback, MatchingDatapoint{dp_id, UyatDatapointType::BOOLEAN}, inverted);
   }

   static DpSwitch create_for_uint(const OnValueCallback& callback, const uint8_t dp_id, const bool inverted = false)
   {
      return DpSwitch(callback, MatchingDatapoint{dp_id, UyatDatapointType::INTEGER}, inverted);
   }

   static DpSwitch create_for_enum(const OnValueCallback& callback, const uint8_t dp_id, const bool inverted = false)
   {
      return DpSwitch(callback, MatchingDatapoint{dp_id, UyatDatapointType::ENUM}, inverted);
   }

   DpSwitch(DpSwitch&&) = default;
   DpSwitch& operator=(DpSwitch&&) = default;

private:

   DpSwitch(const OnValueCallback& callback, MatchingDatapoint&& matching_dp, const bool inverted):
   callback_(callback),
   matching_dp_(std::move(matching_dp)),
   inverted_(inverted)
   {}

   bool invert_if_needed(const bool logical) const
   {
      return this->inverted_? (!logical) : logical;
   }

   OnValueCallback callback_;
   const MatchingDatapoint matching_dp_;
   const bool inverted_;

   DatapointHandler* handler_;
   std::optional<bool> received_value_;
   std::optional<bool> set_value_;
};

}
