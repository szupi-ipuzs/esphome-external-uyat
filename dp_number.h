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

         if (auto * dp_value = std::get_if<BoolDatapointValue>(&datapoint.value))
         {
            this->received_value_ = calculate_logical_value(dp_value->value);
            callback_(received_value_.value());
         }
         else
         if (auto * dp_value = std::get_if<UIntDatapointValue>(&datapoint.value))
         {
            this->received_value_ = calculate_logical_value(dp_value->value);
            callback_(received_value_.value());
         }
         else
         if (auto * dp_value = std::get_if<EnumDatapointValue>(&datapoint.value))
         {
            this->received_value_ = calculate_logical_value(dp_value->value);
            callback_(received_value_.value());
         }
         else
         if (auto * dp_value = std::get_if<Bitmask8DatapointValue>(&datapoint.value))
         {
            this->received_value_ = calculate_logical_value(dp_value->value);
            callback_(received_value_.value());
         }
         else
         if (auto * dp_value = std::get_if<Bitmask16DatapointValue>(&datapoint.value))
         {
            this->received_value_ = calculate_logical_value(dp_value->value);
            callback_(received_value_.value());
         }
         else
         if (auto * dp_value = std::get_if<Bitmask32DatapointValue>(&datapoint.value))
         {
            this->received_value_ = calculate_logical_value(dp_value->value);
            callback_(received_value_.value());
         }
         else
         {
            if (matching_dp_.type.has_value())
            {
               ESP_LOGW(DpNumber::TAG, "Unhandled datapoint type %s!", datapoint.get_type_name());
            }

            return;
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
      uint32_t raw_value = static_cast<uint32_t>((value / this->multiplier_) - float(this->offset_));
      if (this->matching_dp_.type.has_value() == false)
      {
         // assume bitmask
         this->handler_->set_datapoint_value(UyatDatapoint{
            this->matching_dp_.number,
            Bitmask32DatapointValue{raw_value}
         });
      }
      else
      if (this->matching_dp_.type == UyatDatapointType::BOOLEAN)
      {
         this->handler_->set_datapoint_value(UyatDatapoint{
            this->matching_dp_.number,
            BoolDatapointValue{raw_value != 0u}
         });
      }
      else if (this->matching_dp_.type == UyatDatapointType::INTEGER)
      {
         this->handler_->set_datapoint_value(UyatDatapoint{
            this->matching_dp_.number,
            UIntDatapointValue{raw_value}
         });
      }
      else if (this->matching_dp_.type == UyatDatapointType::ENUM)
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
      if (matching_dp_.type)
      {
         return str_sprintf("%s, offset=%d, multiplier=%.2f", this->matching_dp_.to_string().c_str(), this->offset_, this->multiplier_);
      }
      else
      {
         return str_sprintf("Datapoint %u: BITMAP, offset=%d, multiplier=%.2f", this->matching_dp_.number, this->offset_, this->multiplier_);
      }
   }

   static DpNumber create_for_any(const OnValueCallback& callback, const uint8_t dp_id, const int32_t offset = 0, const uint16_t scale = 0)
   {
      // todo: set type to multiple supported types once implemented
      return DpNumber(callback, MatchingDatapoint{dp_id, {}}, offset, scale);
   }

   static DpNumber create_for_bool(const OnValueCallback& callback, const uint8_t dp_id, const int32_t offset = 0, const uint16_t scale = 0)
   {
      return DpNumber(callback, MatchingDatapoint{dp_id, UyatDatapointType::BOOLEAN}, offset, scale);
   }

   static DpNumber create_for_uint(const OnValueCallback& callback, const uint8_t dp_id, const int32_t offset = 0, const uint16_t scale = 0)
   {
      return DpNumber(callback, MatchingDatapoint{dp_id, UyatDatapointType::INTEGER}, offset, scale);
   }

   static DpNumber create_for_enum(const OnValueCallback& callback, const uint8_t dp_id, const int32_t offset = 0, const uint16_t scale = 0)
   {
      return DpNumber(callback, MatchingDatapoint{dp_id, UyatDatapointType::ENUM}, offset, scale);
   }

   static DpNumber create_for_bitmap(const OnValueCallback& callback, const uint8_t dp_id, const int32_t offset = 0, const uint16_t scale = 0)
   {
      // todo: set matching to any bitmask type
      return DpNumber(callback, MatchingDatapoint{dp_id, {}}, offset, scale);
   }

   DpNumber(DpNumber&&) = default;
   DpNumber& operator=(DpNumber&&) = default;

private:

   static constexpr float scale2multiplier(const uint16_t scale)
   {
      switch (scale)
      {
         case 0:
            return 1.0f;
         case 1:
            return 0.1f;
         case 2:
            return 0.01f;
         case 3:
            return 0.001f;
         case 4:
            return 0.0001f;
         case 5:
            return 0.00001f;
         case 6:
            return 0.000001f;
         default:
            return 1.0f;
      }
   }

   DpNumber(const OnValueCallback& callback, MatchingDatapoint&& matching_dp, const int32_t offset, const uint16_t scale):
   callback_(callback),
   matching_dp_(std::move(matching_dp)),
   offset_(offset),
   multiplier_(scale2multiplier(scale))
   {}

   float calculate_logical_value(const uint32_t value) const
   {
      return (float(value) + float(this->offset_)) * this->multiplier_;
   }

   OnValueCallback callback_;
   const MatchingDatapoint matching_dp_;
   const int32_t offset_;
   const float multiplier_;

   DatapointHandler* handler_{nullptr};

   std::optional<float> received_value_;
   std::optional<float> set_value_;
};

}
