#pragma once

#include "esphome/components/number/number.h"
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"

#include "../uyat.h"
#include "../dp_number.h"

namespace esphome {
namespace uyat {

class UyatNumber : public number::Number, public Component {
 private:

  static constexpr const char* TAG = "uyat.number";

  void on_value(const float);

 public:

  struct Config
  {
    MatchingDatapoint matching_dp;
    float offset;
    float multiplier;
  };

  explicit UyatNumber(Uyat *parent, Config config);

  void setup() override;
  void dump_config() override;

 protected:
  void control(float value) override;

  Uyat& parent_;
  DpNumber dp_number_;
};

}  // namespace uyat
}  // namespace esphome
