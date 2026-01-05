#pragma once

#include <functional>
#include <assert.h>

#include "uyat_datapoint_types.h"

namespace esphome::uyat
{

struct DpNumber
{
   static constexpr const char * TAG = "uyat.DpNumber";

   using OnValueCallback = std::function<void(const float)>;

   void init(DatapointHandler& handler)
   {
      handler_ = &handler;
      handler.register_datapoint_listener(this->matching_dp_, [this](const UyatDatapoint &datapoint) {
         ESP_LOGV(DpNumber::TAG, "%s processing as sensor", datapoint.to_string());

         if (!matching_dp_.matches(datapoint.get_type()))
         {
            ESP_LOGW(DpNumber::TAG, "Non-matching datapoint type %s!", datapoint.get_type_name());
            return;
         }

         if (auto * dp_value = std::get_if<BoolDatapointValue>(&datapoint.value))
         {
            if (!this->matching_dp_.allows_single_type())
            {
               this->matching_dp_.types = {UyatDatapointType::BOOLEAN};
               ESP_LOGI(DpNumber::TAG, "Resolved %s", this->matching_dp_.to_string().c_str());
            }
            this->received_value_ = calculate_logical_value(dp_value->value);
            callback_(received_value_.value());
         }
         else
         if (auto * dp_value = std::get_if<UIntDatapointValue>(&datapoint.value))
         {
            if (!this->matching_dp_.allows_single_type())
            {
               this->matching_dp_.types = {UyatDatapointType::INTEGER};
               ESP_LOGI(DpNumber::TAG, "Resolved %s", this->matching_dp_.to_string().c_str());
            }
            this->received_value_ = calculate_logical_value(dp_value->value);
            callback_(received_value_.value());
         }
         else
         if (auto * dp_value = std::get_if<EnumDatapointValue>(&datapoint.value))
         {
            if (!this->matching_dp_.allows_single_type())
            {
               this->matching_dp_.types = {UyatDatapointType::ENUM};
               ESP_LOGI(DpNumber::TAG, "Resolved %s", this->matching_dp_.to_string().c_str());
            }
            this->received_value_ = calculate_logical_value(dp_value->value);
            callback_(received_value_.value());
         }
         else
         if (auto * dp_value = std::get_if<BitmapDatapointValue>(&datapoint.value))
         {
            if (!this->matching_dp_.allows_single_type())
            {
               this->matching_dp_.types = {UyatDatapointType::BITMAP};
               ESP_LOGI(DpNumber::TAG, "Resolved %s", this->matching_dp_.to_string().c_str());
            }
            this->received_value_ = calculate_logical_value(dp_value->value);
            callback_(received_value_.value());
         }
      });
   }

   std::optional<float> get_last_received_value() const
   {
      return this->received_value_;
   }

   void set_value(const float value)
   {
      assert(this->handler_ != nullptr);
      ESP_LOGV(DpNumber::TAG, "Setting value to %.3f for %s", value, this->config_to_string().c_str());
      this->set_value_ = value;
      uint32_t raw_value = static_cast<uint32_t>((lround(value / this->multiplier_) - (this->offset_)));
      if (!this->matching_dp_.allows_single_type())
      {
         ESP_LOGW(DpNumber::TAG, "Cannot set value, datapoint type not yet known for %s", this->matching_dp_.to_string().c_str());
      }
      else
      if (this->matching_dp_.matches(UyatDatapointType::BITMAP))
      {
         this->handler_->set_datapoint_value(UyatDatapoint{
            this->matching_dp_.number,
            BitmapDatapointValue{raw_value}
         });
      }
      else
      if (this->matching_dp_.matches(UyatDatapointType::BOOLEAN))
      {
         this->handler_->set_datapoint_value(UyatDatapoint{
            this->matching_dp_.number,
            BoolDatapointValue{raw_value != 0u}
         });
      }
      else if (this->matching_dp_.matches(UyatDatapointType::INTEGER))
      {
         this->handler_->set_datapoint_value(UyatDatapoint{
            this->matching_dp_.number,
            UIntDatapointValue{raw_value}
         });
      }
      else if (this->matching_dp_.matches(UyatDatapointType::ENUM))
      {
         this->handler_->set_datapoint_value(UyatDatapoint{
            this->matching_dp_.number,
            EnumDatapointValue{static_cast<uint8_t>(raw_value)}
         });
      }
      else
      {
         ESP_LOGW(DpNumber::TAG, "Unhandled datapoint type %s!", this->matching_dp_.to_string().c_str());
      }
   }

   std::string config_to_string() const
   {
      return str_sprintf("%s, offset=%.2f, multiplier=%.2f", this->matching_dp_.to_string().c_str(), this->offset_, this->multiplier_);
   }

   static DpNumber create_for_any(const OnValueCallback& callback, const uint8_t dp_id, const float offset = 0.0f, const float multiplier = 1.0f)
   {
      return DpNumber(callback, MatchingDatapoint{dp_id, {UyatDatapointType::BOOLEAN, UyatDatapointType::INTEGER, UyatDatapointType::ENUM, UyatDatapointType::BITMAP}}, offset, multiplier);
   }

   static DpNumber create_for_bool(const OnValueCallback& callback, const uint8_t dp_id, const float offset = 0.0f, const float multiplier = 1.0f)
   {
      return DpNumber(callback, MatchingDatapoint{dp_id, {UyatDatapointType::BOOLEAN}}, offset, multiplier);
   }

   static DpNumber create_for_uint(const OnValueCallback& callback, const uint8_t dp_id, const float offset = 0.0f, const float multiplier = 1.0f)
   {
      return DpNumber(callback, MatchingDatapoint{dp_id, {UyatDatapointType::INTEGER}}, offset, multiplier);
   }

   static DpNumber create_for_enum(const OnValueCallback& callback, const uint8_t dp_id, const float offset = 0.0f, const float multiplier = 1.0f)
   {
      return DpNumber(callback, MatchingDatapoint{dp_id, {UyatDatapointType::ENUM}}, offset, multiplier);
   }

   static DpNumber create_for_bitmap(const OnValueCallback& callback, const uint8_t dp_id, const float offset = 0.0f, const float multiplier = 1.0f)
   {
      return DpNumber(callback, MatchingDatapoint{dp_id, {UyatDatapointType::BITMAP}}, offset, multiplier);
   }

   DpNumber(DpNumber&&) = default;
   DpNumber& operator=(DpNumber&&) = default;

   DpNumber(const OnValueCallback& callback, MatchingDatapoint&& matching_dp, const float offset, const float multiplier):
   callback_(callback),
   matching_dp_(std::move(matching_dp)),
   offset_(offset),
   multiplier_(multiplier)
   {}

private:

   float calculate_logical_value(const uint32_t value) const
   {
      return (float(value) + this->offset_) * this->multiplier_;
   }

   OnValueCallback callback_;
   MatchingDatapoint matching_dp_;
   const float offset_;
   const float multiplier_;

   DatapointHandler* handler_{nullptr};

   std::optional<float> received_value_;
   std::optional<float> set_value_;
};

}
