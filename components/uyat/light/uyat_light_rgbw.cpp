#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/entity_base.h"

#include "uyat_light_rgbw.h"

namespace esphome::uyat {

UyatLightRGBW::UyatLightRGBW(Uyat *parent, Config config):
parent_(*parent),
dp_switch_{[this](const bool value){ this->on_switch_value(value);},
           std::move(config.switch_config.switch_dp),
           config.switch_config.inverted},
dp_dimmer_{[this](const float brightness_percent){ this->on_dimmer_value(brightness_percent); },
            std::move(config.dimmer_config.dimmer_dp),
            config.dimmer_config.min_value, config.dimmer_config.max_value,
            config.dimmer_config.inverted},
dp_color_{[this](const DpColor::Value& value){ this->on_color_value(value);},
          std::move(config.color_config.color_dp),
          config.color_config.color_type},
color_interlock_(config.color_interlock)
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

void UyatLightRGBW::setup() {
  this->dp_dimmer_.init(this->parent_);
  if (this->dimmer_min_value_)
  {
    this->dimmer_min_value_->init(this->parent_);
    this->dimmer_min_value_->set_value(this->dp_dimmer_.get_config().min_value);
  }

  this->dp_switch_.init(this->parent_);
  this->dp_color_.init(this->parent_);
}

void UyatLightRGBW::dump_config() {
  ESP_LOGCONFIG(UyatLightRGBW::TAG, "Uyat RGBW Light:");
  char config_str[UYAT_LOG_BUFFER_SIZE];
  
  this->dp_switch_.get_config().to_string(config_str, sizeof(config_str));
  ESP_LOGCONFIG(UyatLightRGBW::TAG, "   Switch is %s", config_str);
  
  this->dp_dimmer_.get_config().to_string(config_str, sizeof(config_str));
  ESP_LOGCONFIG(UyatLightRGBW::TAG, "   Dimmer is: %s", config_str);
  
  if (this->dimmer_min_value_)
  {
      this->dimmer_min_value_->get_config().matching_dp.to_string(config_str, sizeof(config_str));
      ESP_LOGCONFIG(UyatLightRGBW::TAG, "   Has min_value_datapoint: %s", config_str);
  }
  
  this->dp_color_.get_config().to_string(config_str, sizeof(config_str));
  ESP_LOGCONFIG(UyatLightRGBW::TAG, "   Color is %s", config_str);
}

light::LightTraits UyatLightRGBW::get_traits() {
  auto traits = light::LightTraits();
  if (this->color_interlock_) {
    traits.set_supported_color_modes({light::ColorMode::RGB, light::ColorMode::WHITE});
  } else {
    traits.set_supported_color_modes({light::ColorMode::RGB_WHITE});
  }
  return traits;
}

void UyatLightRGBW::setup_state(light::LightState *state) { state_ = state; }

void UyatLightRGBW::write_state(light::LightState *state) {
  float red = 0.0f, green = 0.0f, blue = 0.0f;
  float brightness = 0.0f;

  state->current_values_as_rgbw(&red, &green, &blue, &brightness);
  if (!state->current_values.is_on()) {
    this->dp_switch_.set_value(false);
    return;
  }

  if (brightness > 0.0f || !color_interlock_) {
    this->dp_dimmer_.set_value(brightness);
  }

  if (brightness == 0.0f || !color_interlock_) {
    this->dp_color_.set_value(DpColor::Value{red, green, blue});
  }

  this->dp_switch_.set_value(true);
}

void UyatLightRGBW::on_dimmer_value(const float value_percent)
{
  ESP_LOGV(UyatLightRGBW::TAG, "Dimmer of %s reported brightness: %.4f", this->get_name().c_str(), value_percent);
  if (this->state_->current_values != this->state_->remote_values) {
    ESP_LOGD(UyatLightRGBW::TAG, "Light is transitioning, datapoint change ignored");
    return;
  }

  auto call = this->state_->make_call();
  call.set_brightness(value_percent);
  call.perform();
}

void UyatLightRGBW::on_switch_value(const bool value)
{
  ESP_LOGV(UyatLightRGBW::TAG, "MCU reported switch %s is: %s", this->get_name().c_str(), ONOFF(value));
  if (this->state_->current_values != this->state_->remote_values) {
    ESP_LOGD(UyatLightRGBW::TAG, "Light is transitioning, datapoint change ignored");
    return;
  }

  auto call = this->state_->make_call();
  call.set_state(value);
  call.perform();
}

void UyatLightRGBW::on_color_value(const DpColor::Value& value)
{
  ESP_LOGV(UyatLightRGBW::TAG, "MCU reported color %s is: %.2f, %.2f, %.2f", this->get_name().c_str(), value.r, value.g, value.b);

  if (this->state_->current_values != this->state_->remote_values) {
    ESP_LOGD(UyatLightRGBW::TAG, "Light is transitioning, datapoint change ignored");
    return;
  }

  float current_red, current_green, current_blue;
  this->state_->current_values_as_rgb(&current_red, &current_green, &current_blue);
  if (value.r == current_red && value.g == current_green && value.b == current_blue)
    return;
  auto rgb_call = this->state_->make_call();
  rgb_call.set_rgb(value.r, value.g, value.b);
  rgb_call.perform();

}

}  // namespace esphome::uyat
