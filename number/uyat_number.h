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
  void on_value(const float);
  std::string get_object_id() const;

 public:
  void setup() override;
  void dump_config() override;
  void configure_any_dp(const uint8_t dp_id, const uint8_t scale = 0);
  void configure_bool_dp(const uint8_t dp_id, const uint8_t scale = 0);
  void configure_uint_dp(const uint8_t dp_id, const uint8_t scale = 0);
  void configure_enum_dp(const uint8_t dp_id, const uint8_t scale = 0);

  void set_uyat_parent(Uyat *parent) { this->parent_ = parent; }

 protected:
  void control(float value) override;

  Uyat *parent_;
  std::optional<DpNumber> dp_number_;
};

}  // namespace uyat
}  // namespace esphome
