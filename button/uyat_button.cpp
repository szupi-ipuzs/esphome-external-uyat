#include "esphome/core/log.h"
#include "uyat_button.h"

namespace esphome {
namespace uyat {

static const char *const TAG = "uyat.button";

void UyatButton::press_action() {
  ESP_LOGV(TAG, "Pressing button %u", this->switch_id_);
  this->parent_->force_set_boolean_datapoint_value(this->datapoint_id_, this->trigger_payload_);
}

void UyatButton::dump_config() {
  LOG_BUTTON("", "Uyat Button", this);
  ESP_LOGCONFIG(TAG, "  Button has datapoint ID %u, trigger payload is %d", this->datapoint_id_, this->trigger_payload_);
}

}  // namespace uyat
}  // namespace esphome
