#include "esphome/core/log.h"
#include "esphome/core/entity_base.h"
#include "uyat_switch.h"

#include <assert.h>

namespace esphome {
namespace uyat {

static const char *const TAG = "uyat.switch";

void UyatSwitch::setup() {
  assert(this->parent_);
  this->dp_switch_->init(*(this->parent_));
}

void UyatSwitch::write_state(bool state) {
  ESP_LOGV(TAG, "Setting %s to %s", get_object_id().c_str(), ONOFF(state));
  this->dp_switch_->set_value(state);
  this->publish_state(state);
}

void UyatSwitch::dump_config() {
  LOG_SWITCH("", "Uyat Switch", this);
  ESP_LOGCONFIG(TAG, "  Switch %s is %s", get_object_id().c_str(), this->dp_switch_? this->dp_switch_->config_to_string().c_str() : "misconfigured!");
}

void UyatSwitch::on_value(const bool value)
{
  ESP_LOGV(TAG, "MCU reported %s is: %s", get_object_id().c_str(), ONOFF(value));
  this->publish_state(value);
}

std::string UyatSwitch::get_object_id() const
{
  char object_id_buf[OBJECT_ID_MAX_LEN];
  return this->get_object_id_to(object_id_buf).str();
}

}  // namespace uyat
}  // namespace esphome
