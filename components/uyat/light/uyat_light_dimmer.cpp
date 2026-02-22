#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/entity_base.h"

#include "uyat_light_dimmer.h"

namespace esphome::uyat {

UyatLightDimmer::UyatLightDimmer(Uyat *parent, Config config):
parent_(*parent),
dp_switch_{[this](const bool value){ this->on_switch_value(value);},
           std::move(config.switch_config.switch_dp),
           config.switch_config.inverted},
dp_dimmer_{[this](const float brightness_percent){ this->on_dimmer_value(brightness_percent); },
             std::move(config.dimmer_config.dimmer_dp),
             config.dimmer_config.min_value, config.dimmer_config.max_value,
             config.dimmer_config.inverted}
{
  if (config.dimmer_config.min_value_dp)
  {
    this->dimmer_min_value_.emplace(
      [](auto){}, // ignore (write only)
      std::move(*config.dimmer_config.min_value_dp),
      0.0f, 1.0f
    );
  }
}

void UyatLightDimmer::setup() {
  this->dp_dimmer_.init(this->parent_);
  if (this->dimmer_min_value_)
  {
    this->dimmer_min_value_->init(this->parent_);
    this->dimmer_min_value_->set_value(this->dp_dimmer_.get_config().min_value);
  }

  this->dp_switch_.init(this->parent_);
}

void UyatLightDimmer::dump_config() {
  ESP_LOGCONFIG(UyatLightDimmer::TAG, "Uyat Dimmer Light:");
  char config_str[UYAT_LOG_BUFFER_SIZE];
  
  this->dp_switch_.get_config().to_string(config_str, sizeof(config_str));
  ESP_LOGCONFIG(UyatLightDimmer::TAG, "   Switch is %s", config_str);
  
  this->dp_dimmer_.get_config().to_string(config_str, sizeof(config_str));
  ESP_LOGCONFIG(UyatLightDimmer::TAG, "   Dimmer is: %s", config_str);
  
  if (this->dimmer_min_value_)
  {
      this->dimmer_min_value_->get_config().matching_dp.to_string(config_str, sizeof(config_str));
      ESP_LOGCONFIG(UyatLightDimmer::TAG, "   Has min_value_datapoint: %s", config_str);
  }
}

light::LightTraits UyatLightDimmer::get_traits() {
  auto traits = light::LightTraits();
  traits.set_supported_color_modes({light::ColorMode::BRIGHTNESS});
  return traits;
}

void UyatLightDimmer::setup_state(light::LightState *state) { state_ = state; }

void UyatLightDimmer::write_state(light::LightState *state) {
  float brightness = 0.0f;

  state->current_values_as_brightness(&brightness);
  if (!state->current_values.is_on()) {
    this->dp_switch_.set_value(false);
    return;
  }

  this->dp_dimmer_.set_value(brightness);
  this->dp_switch_.set_value(true);
}

void UyatLightDimmer::on_dimmer_value(const float value_percent)
{
  ESP_LOGV(UyatLightDimmer::TAG, "Dimmer of %s reported brightness: %.4f", this->get_name().c_str(), value_percent);
  if (this->state_->current_values != this->state_->remote_values) {
    ESP_LOGD(UyatLightDimmer::TAG, "Light is transitioning, datapoint change ignored");
    return;
  }

  auto call = this->state_->make_call();
  call.set_brightness(value_percent);
  call.perform();
}

void UyatLightDimmer::on_switch_value(const bool value)
{
  ESP_LOGV(UyatLightDimmer::TAG, "MCU reported switch %s is: %s", this->get_name().c_str(), ONOFF(value));
  if (this->state_->current_values != this->state_->remote_values) {
    ESP_LOGD(UyatLightDimmer::TAG, "Light is transitioning, datapoint change ignored");
    return;
  }

  auto call = this->state_->make_call();
  call.set_state(value);
  call.perform();
}

}  // namespace esphome::uyat
