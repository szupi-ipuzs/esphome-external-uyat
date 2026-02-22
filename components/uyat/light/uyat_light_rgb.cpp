#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/entity_base.h"

#include "uyat_light_rgb.h"

namespace esphome::uyat {

UyatLightRGB::UyatLightRGB(Uyat *parent, Config config):
parent_(*parent),
dp_switch_{[this](const bool value){ this->on_switch_value(value);},
           std::move(config.switch_config.switch_dp),
           config.switch_config.inverted},
dp_color_{[this](const DpColor::Value& value){ this->on_color_value(value);},
             std::move(config.color_config.color_dp),
             config.color_config.color_type}
{}

void UyatLightRGB::setup() {
  this->dp_switch_.init(this->parent_);
  this->dp_color_.init(this->parent_);
}

void UyatLightRGB::dump_config() {
  ESP_LOGCONFIG(UyatLightRGB::TAG, "Uyat RGB Light:");
  char config_str[UYAT_LOG_BUFFER_SIZE];
  
  this->dp_switch_.get_config().to_string(config_str, sizeof(config_str));
  ESP_LOGCONFIG(UyatLightRGB::TAG, "   Switch is %s", config_str);
  
  this->dp_color_.get_config().to_string(config_str, sizeof(config_str));
  ESP_LOGCONFIG(UyatLightRGB::TAG, "   Color is %s", config_str);
}

light::LightTraits UyatLightRGB::get_traits() {
  auto traits = light::LightTraits();
  traits.set_supported_color_modes({light::ColorMode::RGB});
  return traits;
}

void UyatLightRGB::setup_state(light::LightState *state) { state_ = state; }

void UyatLightRGB::write_state(light::LightState *state) {
  float red = 0.0f, green = 0.0f, blue = 0.0f;

  state->current_values_as_rgb(&red, &green, &blue);

  if (!state->current_values.is_on()) {
    this->dp_switch_.set_value(false);
    return;
  }

  this->dp_color_.set_value(DpColor::Value{red, green, blue});
  this->dp_switch_.set_value(true);
}

void UyatLightRGB::on_switch_value(const bool value)
{
  ESP_LOGV(UyatLightRGB::TAG, "MCU reported switch %s is: %s", this->get_name().c_str(), ONOFF(value));
  if (this->state_->current_values != this->state_->remote_values) {
    ESP_LOGD(UyatLightRGB::TAG, "Light is transitioning, datapoint change ignored");
    return;
  }

  auto call = this->state_->make_call();
  call.set_state(value);
  call.perform();
}

void UyatLightRGB::on_color_value(const DpColor::Value& value)
{
  ESP_LOGV(UyatLightRGB::TAG, "MCU reported color %s is: %.2f, %.2f, %.2f", this->get_name().c_str(), value.r, value.g, value.b);

  if (this->state_->current_values != this->state_->remote_values) {
    ESP_LOGD(UyatLightRGB::TAG, "Light is transitioning, datapoint change ignored");
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
