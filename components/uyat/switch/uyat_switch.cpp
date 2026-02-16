#include "esphome/core/log.h"
#include "esphome/core/entity_base.h"
#include "uyat_switch.h"

#include <assert.h>

namespace esphome {
namespace uyat {

UyatSwitch::UyatSwitch(Uyat *parent, Config config):
parent_(*parent),
dp_switch_([this](const bool value){this->on_value(value);},
            std::move(config.matching_dp),
            false)
{}

void UyatSwitch::setup() {
  this->dp_switch_.init(this->parent_);
}

void UyatSwitch::write_state(bool state) {
  ESP_LOGV(UyatSwitch::TAG, "Setting %s to %s", get_object_id().c_str(), ONOFF(state));
  this->dp_switch_.set_value(state);
  this->publish_state(state);
}

void UyatSwitch::dump_config() {
  LOG_SWITCH("", "Uyat Switch", this);
  ESP_LOGCONFIG(UyatSwitch::TAG, "  Switch %s is %s", get_object_id().c_str(), this->dp_switch_.get_config().to_string().c_str());
}

void UyatSwitch::on_value(const bool value)
{
  ESP_LOGV(UyatSwitch::TAG, "MCU reported %s is: %s", get_object_id().c_str(), ONOFF(value));
  this->publish_state(value);
}

}  // namespace uyat
}  // namespace esphome
