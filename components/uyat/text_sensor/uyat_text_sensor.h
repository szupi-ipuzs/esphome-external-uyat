#pragma once

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"

#include "../uyat.h"
#include "../dp_text.h"

namespace esphome {
namespace uyat {

class UyatTextSensor : public text_sensor::TextSensor, public Component {
 private:

  static constexpr const char* TAG = "uyat.text_sensor";

  void on_value(const StaticString&);

 public:

  struct Config
  {
    MatchingDatapoint matching_dp;
    TextDataEncoding encoding;
  };

  explicit UyatTextSensor(Uyat *parent, Config config);
  void setup() override;
  void dump_config() override;

 protected:
  Uyat& parent_;
  DpText dp_text_;
};

}  // namespace uyat
}  // namespace esphome
