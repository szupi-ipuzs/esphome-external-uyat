#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/entity_base.h"

#include "uyat_light.h"

namespace esphome::uyat {

static const char *const TAG = "uyat.light";

void UyatLight::setup() {
  if (this->dp_white_temperature_.has_value()) {
    this->dp_white_temperature_->init(*(this->parent_));
  }
  if (this->dimmer_.has_value()) {
    this->dimmer_->dimmer.init(*(this->parent_));
    if (this->dimmer_->min_value_number)
    {
      this->dimmer_->min_value_number->init(*(this->parent_));
      this->dimmer_->min_value_number->set_value(this->dimmer_->dimmer.get_config().min_value);
    }
  }
  if (this->dp_switch_.has_value()) {
    this->dp_switch_->init(*(this->parent_));
  }
  if (dp_color_.has_value()) {
    this->dp_color_->init(*(this->parent_));
  }
}

void UyatLight::dump_config() {
  ESP_LOGCONFIG(TAG, "Uyat Dimmer:");
  if (this->dimmer_.has_value()) {
    ESP_LOGCONFIG(TAG, "   Dimmer is: %s", this->dimmer_->dimmer.get_config().to_string().c_str());
    if (this->dimmer_->min_value_number)
    {
        ESP_LOGCONFIG(TAG, "   Has min_value_datapoint: %s", this->dimmer_->min_value_number->get_config().matching_dp.to_string().c_str());
    }
  }
  if (this->dp_switch_.has_value()) {
    ESP_LOGCONFIG(TAG, "   Switch is %s", this->dp_switch_->get_config().to_string().c_str());
  }
  if (this->dp_color_.has_value()) {
    ESP_LOGCONFIG(TAG, "   Color is %s", this->dp_color_->get_config().to_string().c_str());
  }
  // if (this->dp_white_temperature_)
  // {

  // }
}

light::LightTraits UyatLight::get_traits() {
  auto traits = light::LightTraits();
  if (this->dp_white_temperature_.has_value() && this->dimmer_.has_value()) {
    if (this->dp_color_.has_value()) {
      if (this->color_interlock_) {
        traits.set_supported_color_modes({light::ColorMode::RGB, light::ColorMode::COLOR_TEMPERATURE});
      } else {
        traits.set_supported_color_modes(
            {light::ColorMode::RGB_COLOR_TEMPERATURE, light::ColorMode::COLOR_TEMPERATURE});
      }
    } else {
      traits.set_supported_color_modes({light::ColorMode::COLOR_TEMPERATURE});
    }
    traits.set_min_mireds(this->cold_white_temperature_);
    traits.set_max_mireds(this->warm_white_temperature_);
  } else if (this->dp_color_.has_value()) {
    if (this->dimmer_.has_value()) {
      if (this->color_interlock_) {
        traits.set_supported_color_modes({light::ColorMode::RGB, light::ColorMode::WHITE});
      } else {
        traits.set_supported_color_modes({light::ColorMode::RGB_WHITE});
      }
    } else {
      traits.set_supported_color_modes({light::ColorMode::RGB});
    }
  } else if (this->dimmer_.has_value()) {
    traits.set_supported_color_modes({light::ColorMode::BRIGHTNESS});
  } else {
    traits.set_supported_color_modes({light::ColorMode::ON_OFF});
  }
  return traits;
}

void UyatLight::setup_state(light::LightState *state) { state_ = state; }

void UyatLight::write_state(light::LightState *state) {
  float red = 0.0f, green = 0.0f, blue = 0.0f;
  float color_temperature = 0.0f, brightness = 0.0f;

  if (this->dp_color_.has_value()) {
    if (this->dp_white_temperature_.has_value()) {
      state->current_values_as_rgbct(&red, &green, &blue, &color_temperature, &brightness);
    } else if (this->dimmer_.has_value()) {
      state->current_values_as_rgbw(&red, &green, &blue, &brightness);
    } else {
      state->current_values_as_rgb(&red, &green, &blue);
    }
  } else if (this->dp_white_temperature_.has_value()) {
    state->current_values_as_ct(&color_temperature, &brightness);
  } else {
    state->current_values_as_brightness(&brightness);
  }

  if (!state->current_values.is_on() && this->dp_switch_.has_value()) {
    this->dp_switch_->set_value(false);
    return;
  }

  if (brightness > 0.0f || !color_interlock_) {
    if (this->dp_white_temperature_.has_value()) {
      this->dp_white_temperature_->set_value(color_temperature);
    }

    if (this->dimmer_.has_value()) {
      this->dimmer_->dimmer.set_value(brightness);
    }
  }

  if (this->dp_color_.has_value() && (brightness == 0.0f || !color_interlock_)) {
    this->dp_color_->set_value(DpColor::Value{red, green, blue});
  }

  if (this->dp_switch_.has_value()) {
    this->dp_switch_->set_value(true);
  }
}

void UyatLight::on_dimmer_value(const float value_percent)
{
  ESP_LOGV(TAG, "Dimmer of %s reported brightness: %.4f", get_object_id().c_str(), value_percent);
  if (this->state_->current_values != this->state_->remote_values) {
    ESP_LOGD(TAG, "Light is transitioning, datapoint change ignored");
    return;
  }

  auto call = this->state_->make_call();
  call.set_brightness(value_percent);
  call.perform();
}

void UyatLight::on_switch_value(const bool value)
{
  ESP_LOGV(TAG, "MCU reported switch %s is: %s", get_object_id().c_str(), ONOFF(value));
  if (this->state_->current_values != this->state_->remote_values) {
    ESP_LOGD(TAG, "Light is transitioning, datapoint change ignored");
    return;
  }

  auto call = this->state_->make_call();
  call.set_state(value);
  call.perform();
}

void UyatLight::on_color_value(const DpColor::Value& value)
{
  ESP_LOGV(TAG, "MCU reported color %s is: %.2f, %.2f, %.2f", get_object_id().c_str(), value.r, value.g, value.b);

  if (this->state_->current_values != this->state_->remote_values) {
    ESP_LOGD(TAG, "Light is transitioning, datapoint change ignored");
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

void UyatLight::on_white_temperature_value(const float value_percent)
{
  ESP_LOGV(TAG, "Dimmer of %s reported white temperature: %.4f", get_object_id().c_str(), value_percent);
  if (this->state_->current_values != this->state_->remote_values) {
    ESP_LOGD(TAG, "Light is transitioning, datapoint change ignored");
    return;
  }

  auto call = this->state_->make_call();
  call.set_color_temperature(this->cold_white_temperature_ +
                              (this->warm_white_temperature_ - this->cold_white_temperature_) * value_percent);
  call.perform();
}

}  // namespace esphome::uyat
