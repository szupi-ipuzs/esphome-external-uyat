#include "uyat_climate.h"
#include "esphome/core/log.h"

namespace esphome {
namespace uyat {

static const char *const TAG = "uyat.climate";

void UyatClimate::setup() {
  if (this->switch_id_.has_value()) {
    this->parent_->register_listener(*this->switch_id_, [this](const UyatDatapoint &datapoint) {
      ESP_LOGV(TAG, "MCU reported switch is: %s", ONOFF(datapoint.value_bool));
      this->mode = climate::CLIMATE_MODE_OFF;
      if (datapoint.value_bool) {
        if (this->supports_heat_ && this->supports_cool_) {
          this->mode = climate::CLIMATE_MODE_HEAT_COOL;
        } else if (this->supports_heat_) {
          this->mode = climate::CLIMATE_MODE_HEAT;
        } else if (this->supports_cool_) {
          this->mode = climate::CLIMATE_MODE_COOL;
        }
      }
      this->compute_state_();
      this->publish_state();
    });
  }
  if (this->heating_state_pin_ != nullptr) {
    this->heating_state_pin_->setup();
    this->heating_state_ = this->heating_state_pin_->digital_read();
  }
  if (this->cooling_state_pin_ != nullptr) {
    this->cooling_state_pin_->setup();
    this->cooling_state_ = this->cooling_state_pin_->digital_read();
  }
  if (this->active_state_id_.has_value()) {
    this->parent_->register_listener(*this->active_state_id_, [this](const UyatDatapoint &datapoint) {
      ESP_LOGV(TAG, "MCU reported active state is: %u", datapoint.value_enum);
      this->active_state_ = datapoint.value_enum;
      this->compute_state_();
      this->publish_state();
    });
  }
  if (this->target_temperature_id_.has_value()) {
    this->parent_->register_listener(*this->target_temperature_id_, [this](const UyatDatapoint &datapoint) {
      this->manual_temperature_ = datapoint.value_int * this->target_temperature_multiplier_;
      if (this->reports_fahrenheit_) {
        this->manual_temperature_ = (this->manual_temperature_ - 32) * 5 / 9;
      }

      ESP_LOGV(TAG, "MCU reported manual target temperature is: %.1f", this->manual_temperature_);
      this->compute_target_temperature_();
      this->compute_state_();
      this->publish_state();
    });
  }
  if (this->current_temperature_id_.has_value()) {
    this->parent_->register_listener(*this->current_temperature_id_, [this](const UyatDatapoint &datapoint) {
      this->current_temperature = datapoint.value_int * this->current_temperature_multiplier_;
      if (this->reports_fahrenheit_) {
        this->current_temperature = (this->current_temperature - 32) * 5 / 9;
      }

      ESP_LOGV(TAG, "MCU reported current temperature is: %.1f", this->current_temperature);
      this->compute_state_();
      this->publish_state();
    });
  }
  if (this->eco_id_.has_value()) {
    this->parent_->register_listener(*this->eco_id_, [this](const UyatDatapoint &datapoint) {
      if (this->eco_id_type_ != datapoint.type)
      {
        ESP_LOGW(TAG, "MCU reported different dpid type for eco!");
      }
      this->eco_id_type_ = datapoint.type;
      if (datapoint.type == UyatDatapointType::BOOLEAN)
      {
        this->eco_value_ = datapoint.value_bool;
      }
      else
      if (datapoint.type == UyatDatapointType::ENUM)
      {
        this->eco_value_ = datapoint.value_enum;
      }
      else
      if (datapoint.type == UyatDatapointType::INTEGER)
      {
        this->eco_value_ = datapoint.value_uint;
      }
      ESP_LOGV(TAG, "MCU reported eco is: %u", this->eco_value_);
      this->compute_preset_();
      this->compute_target_temperature_();
      this->publish_state();
    });
  }
  if (this->sleep_id_.has_value()) {
    this->parent_->register_listener(*this->sleep_id_, [this](const UyatDatapoint &datapoint) {
      this->sleep_ = datapoint.value_bool;
      ESP_LOGV(TAG, "MCU reported sleep is: %s", ONOFF(this->sleep_));
      this->compute_preset_();
      this->compute_target_temperature_();
      this->publish_state();
    });
  }
  if (this->swing_vertical_id_.has_value()) {
    this->parent_->register_listener(*this->swing_vertical_id_, [this](const UyatDatapoint &datapoint) {
      this->swing_vertical_ = datapoint.value_bool;
      ESP_LOGV(TAG, "MCU reported vertical swing is: %s", ONOFF(datapoint.value_bool));
      this->compute_swingmode_();
      this->publish_state();
    });
  }

  if (this->swing_horizontal_id_.has_value()) {
    this->parent_->register_listener(*this->swing_horizontal_id_, [this](const UyatDatapoint &datapoint) {
      this->swing_horizontal_ = datapoint.value_bool;
      ESP_LOGV(TAG, "MCU reported horizontal swing is: %s", ONOFF(datapoint.value_bool));
      this->compute_swingmode_();
      this->publish_state();
    });
  }

  if (this->fan_speed_id_.has_value()) {
    this->parent_->register_listener(*this->fan_speed_id_, [this](const UyatDatapoint &datapoint) {
      ESP_LOGV(TAG, "MCU reported Fan Speed Mode is: %u", datapoint.value_enum);
      this->fan_state_ = datapoint.value_enum;
      this->compute_fanmode_();
      this->publish_state();
    });
  }
}

void UyatClimate::loop() {
  bool state_changed = false;
  if (this->heating_state_pin_ != nullptr) {
    bool heating_state = this->heating_state_pin_->digital_read();
    if (heating_state != this->heating_state_) {
      ESP_LOGV(TAG, "Heating state pin changed to: %s", ONOFF(heating_state));
      this->heating_state_ = heating_state;
      state_changed = true;
    }
  }
  if (this->cooling_state_pin_ != nullptr) {
    bool cooling_state = this->cooling_state_pin_->digital_read();
    if (cooling_state != this->cooling_state_) {
      ESP_LOGV(TAG, "Cooling state pin changed to: %s", ONOFF(cooling_state));
      this->cooling_state_ = cooling_state;
      state_changed = true;
    }
  }

  if (state_changed) {
    this->compute_state_();
    this->publish_state();
  }
}

void UyatClimate::control(const climate::ClimateCall &call) {
  if (call.get_mode().has_value()) {
    const bool switch_state = *call.get_mode() != climate::CLIMATE_MODE_OFF;
    ESP_LOGV(TAG, "Setting switch: %s", ONOFF(switch_state));
    this->parent_->set_boolean_datapoint_value(*this->switch_id_, switch_state);
    const climate::ClimateMode new_mode = *call.get_mode();

    if (this->active_state_id_.has_value()) {
      if (new_mode == climate::CLIMATE_MODE_HEAT && this->supports_heat_) {
        this->parent_->set_enum_datapoint_value(*this->active_state_id_, *this->active_state_heating_value_);
      } else if (new_mode == climate::CLIMATE_MODE_COOL && this->supports_cool_) {
        this->parent_->set_enum_datapoint_value(*this->active_state_id_, *this->active_state_cooling_value_);
      } else if (new_mode == climate::CLIMATE_MODE_DRY && this->active_state_drying_value_.has_value()) {
        this->parent_->set_enum_datapoint_value(*this->active_state_id_, *this->active_state_drying_value_);
      } else if (new_mode == climate::CLIMATE_MODE_FAN_ONLY && this->active_state_fanonly_value_.has_value()) {
        this->parent_->set_enum_datapoint_value(*this->active_state_id_, *this->active_state_fanonly_value_);
      }
    } else {
      ESP_LOGW(TAG, "Active state (mode) datapoint not configured");
    }
  }

  control_swing_mode_(call);
  control_fan_mode_(call);

  if (call.get_target_temperature().has_value()) {
    float target_temperature = *call.get_target_temperature();
    if (this->reports_fahrenheit_)
      target_temperature = (target_temperature * 9 / 5) + 32;

    ESP_LOGV(TAG, "Setting target temperature: %.1f", target_temperature);
    this->parent_->set_integer_datapoint_value(*this->target_temperature_id_,
                                               (int) (target_temperature / this->target_temperature_multiplier_));
  }

  if (call.get_preset().has_value()) {
    const climate::ClimatePreset preset = *call.get_preset();
    if (this->eco_id_.has_value()) {
      auto ecoPresetValue = getEcoDpValueForPreset(preset);
      if (ecoPresetValue)
      {
        ESP_LOGV(TAG, "Setting eco: %u", *ecoPresetValue);
        if (this->eco_id_type_ == UyatDatapointType::BOOLEAN)
        {
           this->parent_->set_boolean_datapoint_value(*this->eco_id_, *ecoPresetValue);
        }
        else
        if (this->eco_id_type_ == UyatDatapointType::ENUM)
        {
           this->parent_->set_enum_datapoint_value(*this->eco_id_, *ecoPresetValue);
        }
        else
        if (this->eco_id_type_ == UyatDatapointType::INTEGER)
        {
           this->parent_->set_integer_datapoint_value(*this->eco_id_, *ecoPresetValue);
        }
        else {}
      }
    }
    if (this->sleep_id_.has_value()) {
      const bool sleep = preset == climate::CLIMATE_PRESET_SLEEP;
      ESP_LOGV(TAG, "Setting sleep: %s", ONOFF(sleep));
      this->parent_->set_boolean_datapoint_value(*this->sleep_id_, sleep);
    }
  }
}

void UyatClimate::control_swing_mode_(const climate::ClimateCall &call) {
  bool vertical_swing_changed = false;
  bool horizontal_swing_changed = false;

  if (call.get_swing_mode().has_value()) {
    const auto swing_mode = *call.get_swing_mode();

    switch (swing_mode) {
      case climate::CLIMATE_SWING_OFF:
        if (swing_vertical_ || swing_horizontal_) {
          this->swing_vertical_ = false;
          this->swing_horizontal_ = false;
          vertical_swing_changed = true;
          horizontal_swing_changed = true;
        }
        break;

      case climate::CLIMATE_SWING_BOTH:
        if (!swing_vertical_ || !swing_horizontal_) {
          this->swing_vertical_ = true;
          this->swing_horizontal_ = true;
          vertical_swing_changed = true;
          horizontal_swing_changed = true;
        }
        break;

      case climate::CLIMATE_SWING_VERTICAL:
        if (!swing_vertical_ || swing_horizontal_) {
          this->swing_vertical_ = true;
          this->swing_horizontal_ = false;
          vertical_swing_changed = true;
          horizontal_swing_changed = true;
        }
        break;

      case climate::CLIMATE_SWING_HORIZONTAL:
        if (swing_vertical_ || !swing_horizontal_) {
          this->swing_vertical_ = false;
          this->swing_horizontal_ = true;
          vertical_swing_changed = true;
          horizontal_swing_changed = true;
        }
        break;

      default:
        break;
    }
  }

  if (vertical_swing_changed && this->swing_vertical_id_.has_value()) {
    ESP_LOGV(TAG, "Setting vertical swing: %s", ONOFF(swing_vertical_));
    this->parent_->set_boolean_datapoint_value(*this->swing_vertical_id_, swing_vertical_);
  }

  if (horizontal_swing_changed && this->swing_horizontal_id_.has_value()) {
    ESP_LOGV(TAG, "Setting horizontal swing: %s", ONOFF(swing_horizontal_));
    this->parent_->set_boolean_datapoint_value(*this->swing_horizontal_id_, swing_horizontal_);
  }

  // Publish the state after updating the swing mode
  this->publish_state();
}

void UyatClimate::control_fan_mode_(const climate::ClimateCall &call) {
  if (call.get_fan_mode().has_value()) {
    climate::ClimateFanMode fan_mode = *call.get_fan_mode();

    uint8_t uyat_fan_speed;
    switch (fan_mode) {
      case climate::CLIMATE_FAN_LOW:
        uyat_fan_speed = *fan_speed_low_value_;
        break;
      case climate::CLIMATE_FAN_MEDIUM:
        uyat_fan_speed = *fan_speed_medium_value_;
        break;
      case climate::CLIMATE_FAN_MIDDLE:
        uyat_fan_speed = *fan_speed_middle_value_;
        break;
      case climate::CLIMATE_FAN_HIGH:
        uyat_fan_speed = *fan_speed_high_value_;
        break;
      case climate::CLIMATE_FAN_AUTO:
        uyat_fan_speed = *fan_speed_auto_value_;
        break;
      default:
        uyat_fan_speed = 0;
        break;
    }

    if (this->fan_speed_id_.has_value()) {
      this->parent_->set_enum_datapoint_value(*this->fan_speed_id_, uyat_fan_speed);
    }
  }
}

climate::ClimateTraits UyatClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_action(true);
  traits.set_supports_current_temperature(this->current_temperature_id_.has_value());
  if (supports_heat_)
    traits.add_supported_mode(climate::CLIMATE_MODE_HEAT);
  if (supports_cool_)
    traits.add_supported_mode(climate::CLIMATE_MODE_COOL);
  if (this->active_state_drying_value_.has_value())
    traits.add_supported_mode(climate::CLIMATE_MODE_DRY);
  if (this->active_state_fanonly_value_.has_value())
    traits.add_supported_mode(climate::CLIMATE_MODE_FAN_ONLY);
  if (this->eco_id_.has_value()) {
    if (getEcoDpValueForPreset(climate::CLIMATE_PRESET_ECO))
    {
      traits.add_supported_preset(climate::CLIMATE_PRESET_ECO);
    }
    if (getEcoDpValueForPreset(climate::CLIMATE_PRESET_BOOST))
    {
      traits.add_supported_preset(climate::CLIMATE_PRESET_BOOST);
    }
  }
  if (this->sleep_id_.has_value()) {
    traits.add_supported_preset(climate::CLIMATE_PRESET_SLEEP);
  }
  if (this->sleep_id_.has_value() || this->eco_id_.has_value()) {
    traits.add_supported_preset(climate::CLIMATE_PRESET_NONE);
  }
  if (this->swing_vertical_id_.has_value() && this->swing_horizontal_id_.has_value()) {
    std::set<climate::ClimateSwingMode> supported_swing_modes = {
        climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_BOTH, climate::CLIMATE_SWING_VERTICAL,
        climate::CLIMATE_SWING_HORIZONTAL};
    traits.set_supported_swing_modes(std::move(supported_swing_modes));
  } else if (this->swing_vertical_id_.has_value()) {
    std::set<climate::ClimateSwingMode> supported_swing_modes = {climate::CLIMATE_SWING_OFF,
                                                                 climate::CLIMATE_SWING_VERTICAL};
    traits.set_supported_swing_modes(std::move(supported_swing_modes));
  } else if (this->swing_horizontal_id_.has_value()) {
    std::set<climate::ClimateSwingMode> supported_swing_modes = {climate::CLIMATE_SWING_OFF,
                                                                 climate::CLIMATE_SWING_HORIZONTAL};
    traits.set_supported_swing_modes(std::move(supported_swing_modes));
  }

  if (fan_speed_id_) {
    if (fan_speed_low_value_)
      traits.add_supported_fan_mode(climate::CLIMATE_FAN_LOW);
    if (fan_speed_medium_value_)
      traits.add_supported_fan_mode(climate::CLIMATE_FAN_MEDIUM);
    if (fan_speed_middle_value_)
      traits.add_supported_fan_mode(climate::CLIMATE_FAN_MIDDLE);
    if (fan_speed_high_value_)
      traits.add_supported_fan_mode(climate::CLIMATE_FAN_HIGH);
    if (fan_speed_auto_value_)
      traits.add_supported_fan_mode(climate::CLIMATE_FAN_AUTO);
  }
  return traits;
}

void UyatClimate::dump_config() {
  LOG_CLIMATE("", "Uyat Climate", this);
  if (this->switch_id_.has_value()) {
    ESP_LOGCONFIG(TAG, "  Switch has datapoint ID %u", *this->switch_id_);
  }
  if (this->active_state_id_.has_value()) {
    ESP_LOGCONFIG(TAG, "  Active state has datapoint ID %u", *this->active_state_id_);
  }
  if (this->target_temperature_id_.has_value()) {
    ESP_LOGCONFIG(TAG, "  Target Temperature has datapoint ID %u", *this->target_temperature_id_);
  }
  if (this->current_temperature_id_.has_value()) {
    ESP_LOGCONFIG(TAG, "  Current Temperature has datapoint ID %u", *this->current_temperature_id_);
  }
  LOG_PIN("  Heating State Pin: ", this->heating_state_pin_);
  LOG_PIN("  Cooling State Pin: ", this->cooling_state_pin_);
  if (this->eco_id_.has_value()) {
    ESP_LOGCONFIG(TAG, "  Eco has datapoint ID %u, type %d", *this->eco_id_, this->eco_id_type_);
  }
  if (this->sleep_id_.has_value()) {
    ESP_LOGCONFIG(TAG, "  Sleep has datapoint ID %u", *this->sleep_id_);
  }
  if (this->swing_vertical_id_.has_value()) {
    ESP_LOGCONFIG(TAG, "  Swing Vertical has datapoint ID %u", *this->swing_vertical_id_);
  }
  if (this->swing_horizontal_id_.has_value()) {
    ESP_LOGCONFIG(TAG, "  Swing Horizontal has datapoint ID %u", *this->swing_horizontal_id_);
  }
}

void UyatClimate::compute_preset_() {
  if (getPresetForEcoValue(this->eco_value_)) {
    this->preset = climate::CLIMATE_PRESET_ECO;
  } else if (this->sleep_) {
    this->preset = climate::CLIMATE_PRESET_SLEEP;
  } else {
    this->preset = climate::CLIMATE_PRESET_NONE;
  }
}

void UyatClimate::compute_swingmode_() {
  if (this->swing_vertical_ && this->swing_horizontal_) {
    this->swing_mode = climate::CLIMATE_SWING_BOTH;
  } else if (this->swing_vertical_) {
    this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
  } else if (this->swing_horizontal_) {
    this->swing_mode = climate::CLIMATE_SWING_HORIZONTAL;
  } else {
    this->swing_mode = climate::CLIMATE_SWING_OFF;
  }
}

void UyatClimate::compute_fanmode_() {
  if (this->fan_speed_id_.has_value()) {
    // Use state from MCU datapoint
    if (this->fan_speed_auto_value_.has_value() && this->fan_state_ == this->fan_speed_auto_value_) {
      this->fan_mode = climate::CLIMATE_FAN_AUTO;
    } else if (this->fan_speed_high_value_.has_value() && this->fan_state_ == this->fan_speed_high_value_) {
      this->fan_mode = climate::CLIMATE_FAN_HIGH;
    } else if (this->fan_speed_medium_value_.has_value() && this->fan_state_ == this->fan_speed_medium_value_) {
      this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
    } else if (this->fan_speed_middle_value_.has_value() && this->fan_state_ == this->fan_speed_middle_value_) {
      this->fan_mode = climate::CLIMATE_FAN_MIDDLE;
    } else if (this->fan_speed_low_value_.has_value() && this->fan_state_ == this->fan_speed_low_value_) {
      this->fan_mode = climate::CLIMATE_FAN_LOW;
    }
  }
}

void UyatClimate::compute_target_temperature_() {
  if (this->eco_id_ && this->eco_temperature_.has_value()) {
    this->target_temperature = *this->eco_temperature_;
  } else {
    this->target_temperature = this->manual_temperature_;
  }
}

void UyatClimate::compute_state_() {
  if (std::isnan(this->current_temperature) || std::isnan(this->target_temperature)) {
    // if any control parameters are nan, go to OFF action (not IDLE!)
    this->switch_to_action_(climate::CLIMATE_ACTION_OFF);
    return;
  }

  if (this->mode == climate::CLIMATE_MODE_OFF) {
    this->switch_to_action_(climate::CLIMATE_ACTION_OFF);
    return;
  }

  climate::ClimateAction target_action = climate::CLIMATE_ACTION_IDLE;
  if (this->heating_state_pin_ != nullptr || this->cooling_state_pin_ != nullptr) {
    // Use state from input pins
    if (this->heating_state_) {
      target_action = climate::CLIMATE_ACTION_HEATING;
      this->mode = climate::CLIMATE_MODE_HEAT;
    } else if (this->cooling_state_) {
      target_action = climate::CLIMATE_ACTION_COOLING;
      this->mode = climate::CLIMATE_MODE_COOL;
    }
    if (this->active_state_id_.has_value()) {
      // Both are available, use MCU datapoint as mode
      if (this->supports_heat_ && this->active_state_heating_value_.has_value() &&
          this->active_state_ == this->active_state_heating_value_) {
        this->mode = climate::CLIMATE_MODE_HEAT;
      } else if (this->supports_cool_ && this->active_state_cooling_value_.has_value() &&
                 this->active_state_ == this->active_state_cooling_value_) {
        this->mode = climate::CLIMATE_MODE_COOL;
      } else if (this->active_state_drying_value_.has_value() &&
                 this->active_state_ == this->active_state_drying_value_) {
        this->mode = climate::CLIMATE_MODE_DRY;
      } else if (this->active_state_fanonly_value_.has_value() &&
                 this->active_state_ == this->active_state_fanonly_value_) {
        this->mode = climate::CLIMATE_MODE_FAN_ONLY;
      }
    }
  } else if (this->active_state_id_.has_value()) {
    // Use state from MCU datapoint
    if (this->supports_heat_ && this->active_state_heating_value_.has_value() &&
        this->active_state_ == this->active_state_heating_value_) {
      target_action = climate::CLIMATE_ACTION_HEATING;
      this->mode = climate::CLIMATE_MODE_HEAT;
    } else if (this->supports_cool_ && this->active_state_cooling_value_.has_value() &&
               this->active_state_ == this->active_state_cooling_value_) {
      target_action = climate::CLIMATE_ACTION_COOLING;
      this->mode = climate::CLIMATE_MODE_COOL;
    } else if (this->active_state_drying_value_.has_value() &&
               this->active_state_ == this->active_state_drying_value_) {
      target_action = climate::CLIMATE_ACTION_DRYING;
      this->mode = climate::CLIMATE_MODE_DRY;
    } else if (this->active_state_fanonly_value_.has_value() &&
               this->active_state_ == this->active_state_fanonly_value_) {
      target_action = climate::CLIMATE_ACTION_FAN;
      this->mode = climate::CLIMATE_MODE_FAN_ONLY;
    }
  } else {
    // Fallback to active state calc based on temp and hysteresis
    const float temp_diff = this->target_temperature - this->current_temperature;
    if (std::abs(temp_diff) > this->hysteresis_) {
      if (this->supports_heat_ && temp_diff > 0) {
        target_action = climate::CLIMATE_ACTION_HEATING;
        this->mode = climate::CLIMATE_MODE_HEAT;
      } else if (this->supports_cool_ && temp_diff < 0) {
        target_action = climate::CLIMATE_ACTION_COOLING;
        this->mode = climate::CLIMATE_MODE_COOL;
      }
    }
  }

  this->switch_to_action_(target_action);
}

void UyatClimate::switch_to_action_(climate::ClimateAction action) {
  // For now this just sets the current action but could include triggers later
  this->action = action;
}

optional<uint32_t> UyatClimate::getEcoDpValueForPreset(climate::ClimatePreset preset) const
{
   for (const auto& mapping: this->eco_mapping_)
   {
      if (mapping.second == preset)
      {
        return mapping.first;
      }
   }

   return {};
}

optional<climate::ClimatePreset> UyatClimate::getPresetForEcoValue(uint32_t value) const
{
   for (const auto& mapping: this->eco_mapping_)
   {
      if (mapping.first == value)
      {
        return mapping.second;
      }
   }

   return {};
}

}  // namespace uyat
}  // namespace esphome
