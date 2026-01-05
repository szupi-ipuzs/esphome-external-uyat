#pragma once

#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

#include "../uyat.h"
#include "../dp_binary_sensor.h"

namespace esphome {
namespace uyat {

class UyatBinarySensor : public binary_sensor::BinarySensor, public Component {
 private:
  void on_value(const bool);
  std::string get_object_id() const;

 public:
  void setup() override;
  void dump_config() override;
  void configure(MatchingDatapoint sensor_dp, const uint8_t bit_number = 0){
    this->dp_binary_sensor_.emplace([this](const bool value){this->on_value(value);},
                             std::move(sensor_dp),
                             bit_number,
                             false);
  }

  void set_uyat_parent(Uyat *parent) { this->parent_ = parent; }

 protected:
  Uyat *parent_;
  std::optional<DpBinarySensor> dp_binary_sensor_;
};

}  // namespace uyat
}  // namespace esphome
