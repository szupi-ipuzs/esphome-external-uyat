#pragma once

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"

#include "../uyat.h"
#include "../dp_text.h"

namespace esphome {
namespace uyat {

class UyatTextSensor : public text_sensor::TextSensor, public Component {
 private:
  void on_value(const std::string&);
  std::string get_object_id() const;

 public:
  void setup() override;
  void dump_config() override;
  void configure_any_dp(const uint8_t dp_id, const TextDataEncoding encoding);
  void configure_raw_dp(const uint8_t dp_id, const TextDataEncoding encoding);
  void configure_string_dp(const uint8_t dp_id, const TextDataEncoding encoding);

  void set_uyat_parent(Uyat *parent) { this->parent_ = parent; }

 protected:
  Uyat *parent_;
  std::optional<DpText> dp_text_;
};

}  // namespace uyat
}  // namespace esphome
