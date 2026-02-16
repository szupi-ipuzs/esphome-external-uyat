#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uyat/uyat.h"
#include "esphome/components/button/button.h"

namespace esphome {
namespace uyat {

class UyatButton : public button::Button, public Component {
 public:

  static constexpr const char* TAG = "uyat.button";

  struct Config
  {
    UyatDatapoint trigger_payload;
  };

  explicit UyatButton(Uyat *parent, Config config);
  void dump_config() override;

 protected:
  void press_action() override;

  Uyat& parent_;
  UyatDatapoint trigger_payload_;
};

}  // namespace uyat
}  // namespace esphome
