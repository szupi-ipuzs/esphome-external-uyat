#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/entity_base.h"

#include "uyat_light_ct.h"

namespace esphome::uyat {

UyatLightCT::UyatLightCT(Uyat *parent, Config config):
parent_(*parent),
dp_switch_{[this](const bool value){ this->on_switch_value(value);},
           std::move(config.switch_config.switch_dp),
           config.switch_config.inverted},
dp_dimmer_{[this](const float brightness_percent){ this->on_dimmer_value(brightness_percent); },
             std::move(config.dimmer_config.dimmer_dp),
             config.dimmer_config.min_value, config.dimmer_config.max_value,
             config.dimmer_config.inverted},
dp_white_temperature_{[this](const float brightness_percent){ this->on_white_temperature_value(brightness_percent); },
                      std::move(config.wt_config.white_temperature_dp),
                      config.wt_config.min_value, config.wt_config.max_value,
                      config.wt_config.inverted
                     },
cold_white_temperature_(config.wt_config.cold_white_temperature),
warm_white_temperature_(config.wt_config.warm_white_temperature)
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

void UyatLightCT::setup() {
  this->dp_dimmer_.init(this->parent_);
  if (this->dimmer_min_value_)
  {
    this->dimmer_min_value_->init(this->parent_);
    this->dimmer_min_value_->set_value(this->dp_dimmer_.get_config().min_value);
  }

  this->dp_switch_.init(this->parent_);
  this->dp_white_temperature_.init(this->parent_);
}

void UyatLightCT::dump_config() {
  ESP_LOGCONFIG(UyatLightCT::TAG, "Uyat CT Light:");
  char config_str[UYAT_LOG_BUFFER_SIZE];
  
  this->dp_switch_.get_config().to_string(config_str, sizeof(config_str));
  ESP_LOGCONFIG(UyatLightCT::TAG, "   Switch is %s", config_str);
  
  this->dp_dimmer_.get_config().to_string(config_str, sizeof(config_str));
  ESP_LOGCONFIG(UyatLightCT::TAG, "   Dimmer is: %s", config_str);
  
  if (this->dimmer_min_value_)
  {
      this->dimmer_min_value_->get_config().matching_dp.to_string(config_str, sizeof(config_str));
      ESP_LOGCONFIG(UyatLightCT::TAG, "   Has min_value_datapoint: %s", config_str);
  }
  
  this->dp_white_temperature_.get_config().to_string(config_str, sizeof(config_str));
  ESP_LOGCONFIG(UyatLightCT::TAG, "   CT is: %s", config_str);
}

light::LightTraits UyatLightCT::get_traits() {
  auto traits = light::LightTraits();
  traits.set_supported_color_modes({light::ColorMode::COLOR_TEMPERATURE});
  traits.set_min_mireds(this->cold_white_temperature_);
  traits.set_max_mireds(this->warm_white_temperature_);
  return traits;
}

void UyatLightCT::setup_state(light::LightState *state) { state_ = state; }

void UyatLightCT::write_state(light::LightState *state) {
  float color_temperature = 0.0f, brightness = 0.0f;
  state->current_values_as_ct(&color_temperature, &brightness);
  if (!state->current_values.is_on()) {
    this->dp_switch_.set_value(false);
    return;
  }

  this->dp_white_temperature_.set_value(color_temperature);
  this->dp_dimmer_.set_value(brightness);
  this->dp_switch_.set_value(true);
}

void UyatLightCT::on_dimmer_value(const float value_percent)
{
  ESP_LOGV(UyatLightCT::TAG, "Dimmer of %s reported brightness: %.4f", this->get_name().c_str(), value_percent);
  if (this->state_->current_values != this->state_->remote_values) {
    ESP_LOGD(UyatLightCT::TAG, "Light is transitioning, datapoint change ignored");
    return;
  }

  auto call = this->state_->make_call();
  call.set_brightness(value_percent);
  call.perform();
}

void UyatLightCT::on_switch_value(const bool value)
{
  ESP_LOGV(UyatLightCT::TAG, "MCU reported switch %s is: %s", this->get_name().c_str(), ONOFF(value));
  if (this->state_->current_values != this->state_->remote_values) {
    ESP_LOGD(UyatLightCT::TAG, "Light is transitioning, datapoint change ignored");
    return;
  }

  auto call = this->state_->make_call();
  call.set_state(value);
  call.perform();
}

void UyatLightCT::on_white_temperature_value(const float value_percent)
{
  ESP_LOGV(UyatLightCT::TAG, "Dimmer of %s reported white temperature: %.4f", this->get_name().c_str(), value_percent);
  if (this->state_->current_values != this->state_->remote_values) {
    ESP_LOGD(UyatLightCT::TAG, "Light is transitioning, datapoint change ignored");
    return;
  }

  auto call = this->state_->make_call();
  call.set_color_temperature(this->cold_white_temperature_ +
                              (this->warm_white_temperature_ - this->cold_white_temperature_) * value_percent);
  call.perform();
}

}  // namespace esphome::uyat
