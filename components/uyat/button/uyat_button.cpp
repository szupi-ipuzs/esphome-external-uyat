#include "esphome/core/log.h"
#include "uyat_button.h"

namespace esphome {
namespace uyat {

UyatButton::UyatButton(Uyat *parent, Config config):
parent_(*parent),
trigger_payload_(std::move(config.trigger_payload))
{}

void UyatButton::press_action() {
  char payload_str[UYAT_PRETTY_HEX_BUFFER_SIZE];
  this->trigger_payload_.to_string(payload_str, sizeof(payload_str));
  ESP_LOGV(UyatButton::TAG, "Pressing button %s", payload_str);
  this->parent_.set_datapoint_value(this->trigger_payload_, true);
}

void UyatButton::dump_config() {
  LOG_BUTTON("", "Uyat Button", this);
  char payload_str[UYAT_PRETTY_HEX_BUFFER_SIZE];
  this->trigger_payload_.to_string(payload_str, sizeof(payload_str));
  ESP_LOGCONFIG(UyatButton::TAG, "  Button is %s", payload_str);
}

}  // namespace uyat
}  // namespace esphome
