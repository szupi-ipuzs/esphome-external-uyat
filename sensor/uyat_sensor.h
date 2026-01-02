#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"

#include "../uyat.h"
#include "../dp_number.h"

#include <optional>

namespace esphome::uyat
{

class UyatSensor : public sensor::Sensor, public Component {
 private:
  void on_value(const float);
  std::string get_object_id() const;

 public:
  void setup() override;
  void dump_config() override;
  void configure_any_dp(const uint8_t dp_id);
  void configure_bool_dp(const uint8_t dp_id);
  void configure_uint_dp(const uint8_t dp_id);
  void configure_enum_dp(const uint8_t dp_id);
  void configure_bitmask_dp(const uint8_t dp_id);

  void set_uyat_parent(Uyat *parent) { this->parent_ = parent; }

 protected:
  Uyat *parent_;
  std::optional<DpNumber> dp_number_;
};

}  // namespace
