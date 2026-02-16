#include "esphome/core/log.h"
#include "esphome/core/entity_base.h"

#include "uyat_climate.h"

namespace esphome::uyat
{

UyatClimate::UyatClimate(Uyat *parent, Config config):
parent_(*parent),
supports_heat_(config.supports_heat),
supports_cool_(config.supports_cool)
{
  if (config.switch_config)
  {
    configure_switch(*config.switch_config);
  }
  if (config.active_state_pins_config)
  {
    this->active_state_pins_.heating = config.active_state_pins_config->heating;
    this->active_state_pins_.cooling = config.active_state_pins_config->cooling;
  }
  if (config.active_state_dp_config)
  {
    configure_active_state_dp(*config.active_state_dp_config);
  }
  if (config.temperature_config)
  {
    configure_temperatures(*config.temperature_config);
  }
  if (config.presets_config)
  {
    configure_presets(*config.presets_config);
  }
  if (config.swings_config)
  {
    configure_swings(*config.swings_config);
  }
  if (config.fan_config)
  {
    configure_fan(*config.fan_config);
  }
}

void UyatClimate::on_switch_value(const bool value)
{
  ESP_LOGV(UyatClimate::TAG, "Switch of %s is now %s", this->get_object_id().c_str(), ONOFF(value));
  this->mode = climate::CLIMATE_MODE_OFF;

  if (value)
  {
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
}

void UyatClimate::on_sleep_value(const bool value)
{
  ESP_LOGV(UyatClimate::TAG, "Sleep of %s is now %s", this->get_object_id().c_str(), ONOFF(value));
  this->preset = this->presets_.get_active_preset();
  this->select_target_temperature_to_report_();
  this->publish_state();
}

void UyatClimate::on_eco_value(const bool value)
{
  ESP_LOGV(UyatClimate::TAG, "Eco of %s is now %s", this->get_object_id().c_str(), ONOFF(value));
  this->preset = this->presets_.get_active_preset();
  this->select_target_temperature_to_report_();
  this->publish_state();
}

void UyatClimate::on_boost_value(const bool value)
{
  ESP_LOGV(UyatClimate::TAG, "Boost of %s is now %s", this->get_object_id().c_str(), ONOFF(value));
  this->preset = this->presets_.get_active_preset();
  this->select_target_temperature_to_report_();
  this->publish_state();
}

void UyatClimate::on_target_temperature_value(const float value)
{
  ESP_LOGV(UyatClimate::TAG, "Target temperature of %s reported: %.1f", this->get_object_id().c_str(), value);

  this->select_target_temperature_to_report_();
  this->compute_state_();
  this->publish_state();
}

void UyatClimate::on_current_temperature_value(const float value)
{
  this->current_temperature = *(this->temperatures_->get_current_temperature());

  ESP_LOGV(UyatClimate::TAG, "Current Temperature of %s is now %.1f", this->get_object_id().c_str(), this->current_temperature);
  this->compute_state_();
  this->publish_state();
}

void UyatClimate::on_active_state_value(const float value)
{
  ESP_LOGV(UyatClimate::TAG, "MCU reported active state is: %.0f", value);
  this->compute_state_();
  this->publish_state();
}

void UyatClimate::on_fan_modes_value(const float value)
{
  ESP_LOGV(UyatClimate::TAG, "MCU reported fan speed is: %.0f", value);

  this->compute_fanmode_();
  this->publish_state();
}

void UyatClimate::on_horizontal_swing(const bool value)
{
  ESP_LOGV(UyatClimate::TAG, "MCU reported horizontal swing is: %s", ONOFF(value));
  this->compute_swingmode_();
  this->publish_state();
}

void UyatClimate::on_vertical_swing(const bool value)
{
  ESP_LOGV(UyatClimate::TAG, "MCU reported vertical swing is: %s", ONOFF(value));
  this->compute_swingmode_();
  this->publish_state();
}

void UyatClimate::setup() {
  if (this->dp_switch_.has_value()) {
    this->dp_switch_->init(this->parent_);
  }
  this->active_state_pins_.init();
  if (this->dp_active_state_.has_value()) {
    this->dp_active_state_->dp_number.init(this->parent_);
  }
  if (this->temperatures_.has_value()) {
    this->temperatures_->dp_target.init(this->parent_);
    if (this->temperatures_->dp_current)
    {
      this->temperatures_->dp_current->init(this->parent_);
    }
  }

  this->presets_.init(this->parent_);
  this->swing_modes_.init(this->parent_);

  if (this->fan_modes_.has_value()) {
    this->fan_modes_->dp_number.init(this->parent_);
  }
}

void UyatClimate::loop() {
  if (this->active_state_pins_.update_pins_state()) {
    this->compute_state_();
    this->publish_state();
  }
}

void UyatClimate::control(const climate::ClimateCall &call) {
  if (call.get_mode().has_value()) {
    const bool switch_state = *call.get_mode() != climate::CLIMATE_MODE_OFF;
    ESP_LOGV(UyatClimate::TAG, "Setting switch: %s", ONOFF(switch_state));
    this->dp_switch_->set_value(switch_state);
    const climate::ClimateMode new_mode = *call.get_mode();

    if (this->dp_active_state_.has_value()) {
      if (!this->dp_active_state_->apply_mode(new_mode))
      {
        ESP_LOGW(UyatClimate::TAG, "Failed to apply mode %d!", new_mode);
      }
    } else {
      ESP_LOGW(UyatClimate::TAG, "Active state (mode) datapoint not configured");
    }
  }

  control_swing_mode_(call);
  control_fan_mode_(call);

  if (this->temperatures_.has_value())
  {
    if (call.get_target_temperature().has_value()) {
      this->temperatures_->set_target_temperature(*call.get_target_temperature());
    }
  }

  if (call.get_preset().has_value()) {
    this->presets_.apply_preset(*call.get_preset());
  }
}

void UyatClimate::control_swing_mode_(const climate::ClimateCall &call) {

  if (call.get_swing_mode().has_value()) {
    this->swing_modes_.apply_swing_mode(*call.get_swing_mode());
  }

  // Publish the state after updating the swing mode
  this->publish_state();
}

void UyatClimate::control_fan_mode_(const climate::ClimateCall &call) {
  if ((call.get_fan_mode().has_value() && this->fan_modes_.has_value()))
  {
    this->fan_modes_->apply_mode(*call.get_fan_mode());
  }
}

climate::ClimateTraits UyatClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_ACTION);
  if ((this->temperatures_) && (this->temperatures_->dp_current.has_value())) {
    traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
  }

  if (supports_heat_)
    traits.add_supported_mode(climate::CLIMATE_MODE_HEAT);
  if (supports_cool_)
    traits.add_supported_mode(climate::CLIMATE_MODE_COOL);
  if (this->dp_active_state_->mapping.drying_value.has_value())
    traits.add_supported_mode(climate::CLIMATE_MODE_DRY);
  if (this->dp_active_state_->mapping.fanonly_value.has_value())
    traits.add_supported_mode(climate::CLIMATE_MODE_FAN_ONLY);

  {
    const auto supported_presets = this->presets_.get_supported_presets();
    for (const auto& supported: supported_presets)
    {
      traits.add_supported_preset(supported);
    }
    if (!supported_presets.empty())
    {
      traits.add_supported_preset(climate::CLIMATE_PRESET_NONE);
    }
  }

  {
    const auto supported_modes = this->swing_modes_.get_supported_swing_modes();
    for (const auto& supported: supported_modes)
    {
      traits.add_supported_swing_mode(supported);
    }
    if (!supported_modes.empty())
    {
      traits.add_supported_swing_mode(climate::CLIMATE_SWING_OFF);
    }
  }

  if (this->fan_modes_.has_value()) {
    for (const auto& mode: this->fan_modes_->get_supported_modes())
    {
      traits.add_supported_fan_mode(mode);
    }
  }
  return traits;
}

void UyatClimate::dump_config() {
  LOG_CLIMATE("", "Uyat Climate", this);
  if (this->dp_switch_.has_value()) {
    ESP_LOGCONFIG(UyatClimate::TAG, "  Switch is %s", this->dp_switch_->get_config().to_string().c_str());
  }
  if (this->dp_active_state_.has_value()) {
    this->dp_active_state_->dump_config();
  }
  if (this->temperatures_.has_value()) {
    this->temperatures_->dump_config();
  }
  this->active_state_pins_.dump_config();
  this->presets_.dump_config();
  if (this->swing_modes_.dp_vertical.has_value()) {
    ESP_LOGCONFIG(UyatClimate::TAG, "  Swing Vertical is %s", this->swing_modes_.dp_vertical->get_config().to_string().c_str());
  }
  if (this->swing_modes_.dp_horizontal.has_value()) {
    ESP_LOGCONFIG(UyatClimate::TAG, "  Swing Horizontal is %s", this->swing_modes_.dp_horizontal->get_config().to_string().c_str());
  }
  if (this->fan_modes_.has_value()) {
    ESP_LOGCONFIG(UyatClimate::TAG, "  Fan Speed is %s", this->fan_modes_->dp_number.get_config().to_string().c_str());
  }
}

void UyatClimate::compute_swingmode_() {
  this->swing_mode = this->swing_modes_.get_current_swing_mode();
}

void UyatClimate::compute_fanmode_() {
  if (this->fan_modes_.has_value()) {
    const auto current_mode = this->fan_modes_->get_current_mode();
    if (current_mode.has_value())
    {
      this->fan_mode = *current_mode;
    }
  }
}

void UyatClimate::select_target_temperature_to_report_() {
  if (auto preset_temperature = this->presets_.get_active_preset_temperature())
  {
    this->target_temperature = *preset_temperature;
  }
  else
  {
    if (this->temperatures_)
    {
      const auto manual_temperature = this->temperatures_->get_target_temperature();
      if (manual_temperature)
      {
        this->target_temperature = *manual_temperature;
      }
    }
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
  if (auto mode_from_pins = this->active_state_pins_.mode_from_state()) {
    this->mode = *mode_from_pins;
    if (this->mode == climate::CLIMATE_MODE_HEAT)
    {
       target_action = climate::CLIMATE_ACTION_HEATING;
    }
    else
    if (this->mode == climate::CLIMATE_MODE_COOL)
    {
       target_action = climate::CLIMATE_ACTION_COOLING;
    }

    if (this->dp_active_state_.has_value()) {
      const auto mode_from_dp = this->dp_active_state_->last_value_to_mode();
      if (mode_from_dp)
      {
        this->mode = *mode_from_dp;
      }
    }
  } else if (this->dp_active_state_.has_value()) {
    {
     // Use state & action from MCU datapoint
      const auto mode_from_dp = this->dp_active_state_->last_value_to_mode();
      if (mode_from_dp)
      {
        this->mode = *mode_from_dp;
      }
    }
    {
      const auto action_from_dp = this->dp_active_state_->last_value_to_action();
      if (action_from_dp)
      {
        target_action = *action_from_dp;
      }
    }
  } else {
    if (this->temperatures_.has_value())
    {
      // Fallback to active state calc based on temp and hysteresis
      const float temp_diff = this->target_temperature - this->current_temperature;
      if (std::abs(temp_diff) >= this->temperatures_->hysteresis) {
        if (this->supports_heat_ && temp_diff > 0) {
          target_action = climate::CLIMATE_ACTION_HEATING;
          this->mode = climate::CLIMATE_MODE_HEAT;
        } else if (this->supports_cool_ && temp_diff < 0) {
          target_action = climate::CLIMATE_ACTION_COOLING;
          this->mode = climate::CLIMATE_MODE_COOL;
        }
      }
    }
  }

  this->switch_to_action_(target_action);
}

void UyatClimate::switch_to_action_(climate::ClimateAction action) {
  // For now this just sets the current action but could include triggers later
  this->action = action;
}

}  // namespace esphome::uyat
