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

  static constexpr const char* TAG = "uyat.sensor";

  void on_value(const float);

 public:

  struct Config
  {
    MatchingDatapoint matching_dp;
  };

  explicit UyatSensor(Uyat *parent, Config config);

  void setup() override;
  void dump_config() override;

 protected:
  Uyat& parent_;
  DpNumber dp_number_;
};

}  // namespace
