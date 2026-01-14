#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"

#include "../uyat.h"
#include "../dp_number.h"

#include <optional>

namespace esphome::uyat
{

class UyatSensor : public sensor::Sensor, public Component {
 private:
  void on_value(const float);

 public:
  void setup() override;
  void dump_config() override;
  void configure(MatchingDatapoint number_dp){
    this->dp_number_.emplace([this](const float value){
      this->on_value(value);
    },
    std::move(number_dp),
    0, 1.0f);
  }

  void set_uyat_parent(Uyat *parent) { this->parent_ = parent; }

 protected:
  Uyat *parent_;
  std::optional<DpNumber> dp_number_;
};

}  // namespace
