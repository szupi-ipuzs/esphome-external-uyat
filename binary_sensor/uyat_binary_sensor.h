#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uyat/uyat.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome {
namespace uyat {

class UyatBinarySensor : public binary_sensor::BinarySensor, public Component {
 public:
  void setup() override;
  void dump_config() override;
  void set_matching_dp(const MatchingDatapoint& matching_dp) { this->matching_dp_ = matching_dp; }

  void set_uyat_parent(Uyat *parent) { this->parent_ = parent; }

 protected:
  Uyat *parent_;
  MatchingDatapoint matching_dp_;
};

}  // namespace uyat
}  // namespace esphome
