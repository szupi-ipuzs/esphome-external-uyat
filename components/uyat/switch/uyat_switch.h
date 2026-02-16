#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uyat/uyat.h"
#include "esphome/components/switch/switch.h"

#include "../dp_switch.h"

namespace esphome {
namespace uyat {

class UyatSwitch : public switch_::Switch, public Component {
 private:

  static constexpr const char* TAG = "uyat.switch";

  void on_value(const bool);

 public:

  struct Config
  {
    MatchingDatapoint matching_dp;
  };

  explicit UyatSwitch(Uyat *parent, Config config);
  void setup() override;
  void dump_config() override;

 protected:
  void write_state(bool state) override;

  Uyat& parent_;
  DpSwitch dp_switch_;
};

}  // namespace uyat
}  // namespace esphome
