#pragma once

#include <functional>

#include "uyat_datapoint_types.h"

namespace esphome::uyat
{

struct DpVAP
{
   static constexpr const char * TAG = "uyat.DpVAP";

   struct VAPValue {
      uint32_t v;
      uint32_t a;
      uint32_t p;

      std::string to_string() const {
         return str_sprintf("V: %u, A: %u, P: %u", v, a, p);
      }
   };
   using OnValueCallback = std::function<void(const VAPValue&)>;

   void init(DatapointHandler& handler)
   {
      handler_ = &handler;
      handler.register_datapoint_listener(this->matching_dp_, [this](const UyatDatapoint &datapoint) {
         ESP_LOGV(DpVAP::TAG, "%s processing as VAP", datapoint.to_string().c_str());

         if (!matching_dp_.matches(datapoint.get_type()))
         {
            ESP_LOGW(DpVAP::TAG, "Non-matching datapoint type %s!", datapoint.get_type_name());
            return;
         }

         if (auto * dp_value = std::get_if<RawDatapointValue>(&datapoint.value))
         {
            if (auto decoded = decode_(dp_value->value))
            {
               this->received_value_ = decoded;
               callback_(received_value_.value());
            }
            else
            {
               ESP_LOGW(DpVAP::TAG, "Failed to decode VAP from datapoint %s", datapoint.to_string().c_str());
            }
         }
         else
         {
            ESP_LOGW(DpVAP::TAG, "Unhandled datapoint type %s!", datapoint.get_type_name());
            return;
         }
      });
   }

   std::optional<VAPValue> get_last_received_value() const
   {
      return this->received_value_;
   }

   std::string config_to_string() const
   {
      return this->matching_dp_.to_string();
   }

   DpVAP(DpVAP&&) = default;
   DpVAP& operator=(DpVAP&&) = default;

   DpVAP(const OnValueCallback& callback, MatchingDatapoint&& matching_dp):
   callback_(callback),
   matching_dp_(std::move(matching_dp))
   {}

private:

   std::optional<VAPValue> decode_(const std::vector<uint8_t>& raw_data) const
   {
      if (raw_data.size() != 8u)
      {
         return std::nullopt;
      }

      return VAPValue{
         .v = {((static_cast<uint32_t>(raw_data[0]) << 8) | raw_data[1])},
         .a = {((static_cast<uint32_t>(raw_data[3]) << 8) | raw_data[4])},
         .p = {((static_cast<uint32_t>(raw_data[6]) << 8) | raw_data[7])},
      };
   }

   OnValueCallback callback_;
   const MatchingDatapoint matching_dp_;

   DatapointHandler* handler_{nullptr};

   std::optional<VAPValue> received_value_;
};

}
