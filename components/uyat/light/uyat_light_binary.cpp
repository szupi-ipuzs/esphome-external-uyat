#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/entity_base.h"

#include "uyat_light_binary.h"

namespace esphome::uyat {

UyatLightBinary::UyatLightBinary(Uyat *parent, Config config):
parent_(*parent),
dp_switch_{[this](const bool value){ this->on_switch_value(value);},
           std::move(config.switch_config.switch_dp),
           config.switch_config.inverted}
{}

void UyatLightBinary::setup() {
  this->dp_switch_.init(this->parent_);
}

void UyatLightBinary::dump_config() {
  ESP_LOGCONFIG(UyatLightBinary::TAG, "Uyat Binary Light:");
  char config_str[UYAT_LOG_BUFFER_SIZE];
  
  this->dp_switch_.get_config().to_string(config_str, sizeof(config_str));
  ESP_LOGCONFIG(UyatLightBinary::TAG, "   Switch is %s", config_str);
}

light::LightTraits UyatLightBinary::get_traits() {
  auto traits = light::LightTraits();
  traits.set_supported_color_modes({light::ColorMode::ON_OFF});
  return traits;
}

void UyatLightBinary::setup_state(light::LightState *state) { state_ = state; }

void UyatLightBinary::write_state(light::LightState *state) {
  this->dp_switch_.set_value(state->current_values.is_on());
}

void UyatLightBinary::on_switch_value(const bool value)
{
  ESP_LOGV(UyatLightBinary::TAG, "MCU reported switch %s is: %s", this->get_name().c_str(), ONOFF(value));
  if (this->state_->current_values != this->state_->remote_values) {
    ESP_LOGD(UyatLightBinary::TAG, "Light is transitioning, datapoint change ignored");
    return;
  }

  auto call = this->state_->make_call();
  call.set_state(value);
  call.perform();
}

}  // namespace esphome::uyat
