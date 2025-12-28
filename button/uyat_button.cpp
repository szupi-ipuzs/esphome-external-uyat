#include "esphome/core/log.h"
#include "uyat_button.h"

namespace esphome {
namespace uyat {

static const char *const TAG = "uyat.button";

void UyatButton::press_action() {
  ESP_LOGV(TAG, "Pressing button %s", this->trigger_payload_.to_string().c_str());
  this->parent_->set_datapoint_value(this->trigger_payload_, true);
}

void UyatButton::dump_config() {
  LOG_BUTTON("", "Uyat Button", this);
  ESP_LOGCONFIG(TAG, "  Button is %s", this->trigger_payload_.to_string().c_str());
}

}  // namespace uyat
}  // namespace esphome
