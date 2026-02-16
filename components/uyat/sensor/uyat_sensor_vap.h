#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"

#include "../uyat.h"
#include "../dp_vap.h"

#include <optional>

namespace esphome::uyat
{

enum class UyatVAPValueType {
  VOLTAGE,
  AMPERAGE,
  POWER,
};

class UyatSensorVAP : public sensor::Sensor, public Component {
 private:

  static constexpr const char* TAG = "uyat.sensorVAP";

  void on_value(const DpVAP::VAPValue&);

 public:

  struct Config
  {
    MatchingDatapoint matching_dp;
    UyatVAPValueType value_type;
  };

  explicit UyatSensorVAP(Uyat *parent, Config config);
  void setup() override;
  void dump_config() override;

 protected:
  Uyat& parent_;
  DpVAP dp_vap_;
  UyatVAPValueType value_type_;
};

}  // namespace
