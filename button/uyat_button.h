#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uyat/uyat.h"
#include "esphome/components/button/button.h"

namespace esphome {
namespace uyat {

class UyatButton : public button::Button, public Component {
 public:
  void dump_config() override;
  void set_datapoint_id(const uint8_t datapoint_id) { this->datapoint_id_ = datapoint_id; }
  void set_trigger_payload(const bool trigger_payload) { this->trigger_payload_ = trigger_payload; }

  void set_uyat_parent(Uyat *parent) { this->parent_ = parent; }

 protected:
  void press_action() override;

  Uyat *parent_;
  uint8_t datapoint_id_{0};
  bool trigger_payload_{true};
};

}  // namespace uyat
}  // namespace esphome
