#include "esphome/core/log.h"
#include "uyat_light.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace uyat {

static const char *const TAG = "uyat.light";

void UyatLight::setup() {
  if (this->color_temperature_id_.has_value()) {
    this->parent_->register_listener(*this->color_temperature_id_, [this](const UyatDatapoint &datapoint) {
      if (this->state_->current_values != this->state_->remote_values) {
        ESP_LOGD(TAG, "Light is transitioning, datapoint change ignored");
        return;
      }

      auto * dp_value = std::get_if<UIntDatapointValue>(&datapoint.value);
      if (!dp_value)
      {
        ESP_LOGW(TAG, "Unexpected datapoint type!");
        return;
      }

      auto datapoint_value = dp_value->value;
      if (this->color_temperature_invert_) {
        datapoint_value = this->color_temperature_max_value_ - datapoint_value;
      }
      auto call = this->state_->make_call();
      call.set_color_temperature(this->cold_white_temperature_ +
                                 (this->warm_white_temperature_ - this->cold_white_temperature_) *
                                     (float(datapoint_value) / this->color_temperature_max_value_));
      call.perform();
    });
  }
  if (this->dimmer_id_.has_value()) {
    this->parent_->register_listener(*this->dimmer_id_, [this](const UyatDatapoint &datapoint) {
      if (this->state_->current_values != this->state_->remote_values) {
        ESP_LOGD(TAG, "Light is transitioning, datapoint change ignored");
        return;
      }

      auto * dp_value = std::get_if<UIntDatapointValue>(&datapoint.value);
      if (!dp_value)
      {
        ESP_LOGW(TAG, "Unexpected datapoint type!");
        return;
      }

      auto call = this->state_->make_call();
      call.set_brightness(float(dp_value->value) / this->max_value_);
      call.perform();
    });
  }
  if (switch_id_.has_value()) {
    this->parent_->register_listener(*this->switch_id_, [this](const UyatDatapoint &datapoint) {
      if (this->state_->current_values != this->state_->remote_values) {
        ESP_LOGD(TAG, "Light is transitioning, datapoint change ignored");
        return;
      }

      auto * dp_value = std::get_if<BoolDatapointValue>(&datapoint.value);
      if (!dp_value)
      {
        ESP_LOGW(TAG, "Unexpected datapoint type!");
        return;
      }

      auto call = this->state_->make_call();
      call.set_state(dp_value->value);
      call.perform();
    });
  }
  if (color_id_.has_value()) {
    this->parent_->register_listener(*this->color_id_, [this](const UyatDatapoint &datapoint) {
      if (this->state_->current_values != this->state_->remote_values) {
        ESP_LOGD(TAG, "Light is transitioning, datapoint change ignored");
        return;
      }

      auto * dp_value = std::get_if<StringDatapointValue>(&datapoint.value);
      if (!dp_value)
      {
        ESP_LOGW(TAG, "Unexpected datapoint type!");
        return;
      }

      float red, green, blue;
      switch (*this->color_type_) {
        case UyatColorType::RGBHSV:
        case UyatColorType::RGB: {
          auto rgb = parse_hex<uint32_t>(dp_value->value.substr(0, 6));
          if (!rgb.has_value())
            return;

          red = (*rgb >> 16) / 255.0f;
          green = ((*rgb >> 8) & 0xff) / 255.0f;
          blue = (*rgb & 0xff) / 255.0f;
          break;
        }
        case UyatColorType::HSV: {
          auto hue = parse_hex<uint16_t>(dp_value->value.substr(0, 4));
          auto saturation = parse_hex<uint16_t>(dp_value->value.substr(4, 4));
          auto value = parse_hex<uint16_t>(dp_value->value.substr(8, 4));
          if (!hue.has_value() || !saturation.has_value() || !value.has_value())
            return;

          hsv_to_rgb(*hue, float(*saturation) / 1000, float(*value) / 1000, red, green, blue);
          break;
        }
      }

      float current_red, current_green, current_blue;
      this->state_->current_values_as_rgb(&current_red, &current_green, &current_blue);
      if (red == current_red && green == current_green && blue == current_blue)
        return;
      auto rgb_call = this->state_->make_call();
      rgb_call.set_rgb(red, green, blue);
      rgb_call.perform();
    });
  }

  if (min_value_datapoint_id_.has_value()) {
    this->parent_->set_integer_datapoint_value(*this->min_value_datapoint_id_, this->min_value_);
  }
}

void UyatLight::dump_config() {
  ESP_LOGCONFIG(TAG, "Uyat Dimmer:");
  if (this->dimmer_id_.has_value()) {
    ESP_LOGCONFIG(TAG, "   Dimmer has datapoint ID %u", *this->dimmer_id_);
  }
  if (this->switch_id_.has_value()) {
    ESP_LOGCONFIG(TAG, "   Switch has datapoint ID %u", *this->switch_id_);
  }
  if (this->color_id_.has_value()) {
    ESP_LOGCONFIG(TAG, "   Color has datapoint ID %u", *this->color_id_);
  }
}

light::LightTraits UyatLight::get_traits() {
  auto traits = light::LightTraits();
  if (this->color_temperature_id_.has_value() && this->dimmer_id_.has_value()) {
    if (this->color_id_.has_value()) {
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
  } else if (this->color_id_.has_value()) {
    if (this->dimmer_id_.has_value()) {
      if (this->color_interlock_) {
        traits.set_supported_color_modes({light::ColorMode::RGB, light::ColorMode::WHITE});
      } else {
        traits.set_supported_color_modes({light::ColorMode::RGB_WHITE});
      }
    } else {
      traits.set_supported_color_modes({light::ColorMode::RGB});
    }
  } else if (this->dimmer_id_.has_value()) {
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

  if (this->color_id_.has_value()) {
    if (this->color_temperature_id_.has_value()) {
      state->current_values_as_rgbct(&red, &green, &blue, &color_temperature, &brightness);
    } else if (this->dimmer_id_.has_value()) {
      state->current_values_as_rgbw(&red, &green, &blue, &brightness);
    } else {
      state->current_values_as_rgb(&red, &green, &blue);
    }
  } else if (this->color_temperature_id_.has_value()) {
    state->current_values_as_ct(&color_temperature, &brightness);
  } else {
    state->current_values_as_brightness(&brightness);
  }

  if (!state->current_values.is_on() && this->switch_id_.has_value()) {
    this->parent_->set_boolean_datapoint_value(*this->switch_id_, false);
    return;
  }

  if (brightness > 0.0f || !color_interlock_) {
    if (this->color_temperature_id_.has_value()) {
      uint32_t color_temp_int = static_cast<uint32_t>(roundf(color_temperature * this->color_temperature_max_value_));
      if (this->color_temperature_invert_) {
        color_temp_int = this->color_temperature_max_value_ - color_temp_int;
      }
      ESP_LOGD(TAG, "Setting color temperature datapoint %d, color_temperature %.3f, color_temperature_max_value %d, color_temp_int %d", *this->color_temperature_id_, color_temperature, this->color_temperature_max_value_, color_temp_int);
      this->parent_->set_integer_datapoint_value(*this->color_temperature_id_, color_temp_int);
    }

    if (this->dimmer_id_.has_value()) {
      auto brightness_int = static_cast<uint32_t>(brightness * this->max_value_);
      brightness_int = std::max(brightness_int, this->min_value_);

      ESP_LOGD(TAG, "Setting dimmer datapoint %d, brightness %.3f, max_value %d, brightness_int %d", *this->dimmer_id_, brightness, this->max_value_, brightness_int);
      this->parent_->set_integer_datapoint_value(*this->dimmer_id_, brightness_int);
    }
  }

  if (this->color_id_.has_value() && (brightness == 0.0f || !color_interlock_)) {
    std::string color_value;
    switch (*this->color_type_) {
      case UyatColorType::RGB: {
        char buffer[7];
        sprintf(buffer, "%02X%02X%02X", int(red * 255), int(green * 255), int(blue * 255));
        color_value = buffer;
        break;
      }
      case UyatColorType::HSV: {
        int hue;
        float saturation, value;
        rgb_to_hsv(red, green, blue, hue, saturation, value);
        char buffer[13];
        sprintf(buffer, "%04X%04X%04X", hue, int(saturation * 1000), int(value * 1000));
        color_value = buffer;
        break;
      }
      case UyatColorType::RGBHSV: {
        int hue;
        float saturation, value;
        rgb_to_hsv(red, green, blue, hue, saturation, value);
        char buffer[15];
        sprintf(buffer, "%02X%02X%02X%04X%02X%02X", int(red * 255), int(green * 255), int(blue * 255), hue,
                int(saturation * 255), int(value * 255));
        color_value = buffer;
        break;
      }
    }
    this->parent_->set_string_datapoint_value(*this->color_id_, color_value);
  }

  if (this->switch_id_.has_value()) {
    this->parent_->set_boolean_datapoint_value(*this->switch_id_, true);
  }
}

}  // namespace uyat
}  // namespace esphome
