#pragma once

#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

#include "../uyat.h"
#include "../dp_binary_sensor.h"

namespace esphome {
namespace uyat {

class UyatBinarySensor : public binary_sensor::BinarySensor, public Component {
 private:

  static constexpr const char* TAG = "uyat.binary_sensor";

  void on_value(const bool);

 public:

  struct Config
  {
    MatchingDatapoint sensor_dp;
    std::optional<uint8_t> bit_number;
  };

  explicit UyatBinarySensor(Uyat* parent, Config config);

  void setup() override;
  void dump_config() override;

 protected:
  Uyat& parent_;
  DpBinarySensor dp_binary_sensor_;
};

}  // namespace uyat
}  // namespace esphome
