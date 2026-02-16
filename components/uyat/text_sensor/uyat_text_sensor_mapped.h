#pragma once

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"

#include "../uyat.h"
#include "../dp_number.h"

namespace esphome {
namespace uyat {

class UyatTextSensorMapped : public text_sensor::TextSensor, public Component {
 private:

  static constexpr const char* TAG = "uyat.text_sensor_mapped";

  void on_value(const float);
  std::string translate(const uint32_t number_value) const;

 public:

  struct Config
  {
    MatchingDatapoint matching_dp;
    std::vector<std::pair<uint32_t, std::string>> mapping;
  };

  explicit UyatTextSensorMapped(Uyat *parent, Config config);
  void setup() override;
  void dump_config() override;

 protected:
  Uyat& parent_;
  DpNumber dp_number_;
  const std::vector<std::pair<uint32_t, std::string>> mapping_;
};

}  // namespace uyat
}  // namespace esphome
