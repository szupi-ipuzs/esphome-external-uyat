#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uyat/uyat.h"
#include "esphome/components/button/button.h"

namespace esphome {
namespace uyat {

class UyatButton : public button::Button, public Component {
 public:
  void dump_config() override;
  void set_trigger_payload(const UyatDatapoint& trigger_payload) { this->trigger_payload_ = trigger_payload; }

  void set_uyat_parent(Uyat *parent) { this->parent_ = parent; }

 protected:
  void press_action() override;

  Uyat *parent_;
  UyatDatapoint trigger_payload_;
};

}  // namespace uyat
}  // namespace esphome
