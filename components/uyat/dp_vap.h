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

      void to_string(char* buffer, size_t size) const {
         snprintf(buffer, size, "V: %u, A: %u, P: %u", v, a, p);
      }
   };
   using OnValueCallback = std::function<void(const VAPValue&)>;

   struct Config
   {
      MatchingDatapoint matching_dp;

      void to_string(char* buffer, size_t size) const
      {
         this->matching_dp.to_string(buffer, size);
      }
   };

   void init(DatapointHandler& handler)
   {
      handler_ = &handler;
      handler.register_datapoint_listener(this->config_.matching_dp, [this](const UyatDatapoint &datapoint) {
         char datapoint_str[UYAT_PRETTY_HEX_BUFFER_SIZE];
         datapoint.to_string(datapoint_str, sizeof(datapoint_str));
         ESP_LOGV(DpVAP::TAG, "%s processing as VAP", datapoint_str);

         if (!this->config_.matching_dp.matches(datapoint.get_type()))
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
               ESP_LOGW(DpVAP::TAG, "Failed to decode VAP from datapoint %s", datapoint_str);
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

   const Config& get_config() const
   {
      return config_;
   }

   DpVAP(DpVAP&&) = default;
   DpVAP& operator=(DpVAP&&) = default;

   DpVAP(const OnValueCallback& callback, MatchingDatapoint&& matching_dp):
   config_{std::move(matching_dp)},
   callback_(callback)
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

   Config config_;
   OnValueCallback callback_;

   DatapointHandler* handler_{nullptr};

   std::optional<VAPValue> received_value_;
};

}
