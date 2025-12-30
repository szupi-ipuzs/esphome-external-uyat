#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uyat/uyat.h"
#include "esphome/components/switch/switch.h"

#include "../dp_switch.h"

namespace esphome {
namespace uyat {

class UyatSwitch : public switch_::Switch, public Component {
 private:
  void on_value(const bool);
  std::string get_object_id() const;

 public:
  void setup() override;
  void dump_config() override;
  void configure_bool_dp(const uint8_t dp_id);
  void configure_uint_dp(const uint8_t dp_id);
  void configure_enum_dp(const uint8_t dp_id);

  void set_uyat_parent(Uyat *parent) { this->parent_ = parent; }

 protected:
  void write_state(bool state) override;

  Uyat *parent_;
  std::optional<DpSwitch> dp_switch_;
};

}  // namespace uyat
}  // namespace esphome
