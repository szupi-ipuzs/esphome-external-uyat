#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"

#include "../uyat.h"
#include "../dp_switch.h"
#include "../dp_number.h"

namespace esphome {
namespace uyat {

class UyatClimate : public climate::Climate, public Component {
 private:
  void on_switch_value(const bool);
  void on_sleep_value(const bool);
  void on_eco_value(const bool);
  void on_target_temperature_value(const float);
  void on_current_temperature_value(const float);
  std::string get_object_id() const;

 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  void set_supports_heat(bool supports_heat) { this->supports_heat_ = supports_heat; }
  void set_supports_cool(bool supports_cool) { this->supports_cool_ = supports_cool; }
  void set_switch_id(MatchingDatapoint switch_dp, const bool inverted = false) {
    this->dp_switch_.emplace([this](const bool value){this->on_switch_value(value);},
                             std::move(switch_dp),
                             inverted);
  }
  void set_active_state_id(const MatchingDatapoint& state_id) { this->active_state_id_ = state_id; }
  void set_active_state_heating_value(uint8_t value) { this->active_state_heating_value_ = value; }
  void set_active_state_cooling_value(uint8_t value) { this->active_state_cooling_value_ = value; }
  void set_active_state_drying_value(uint8_t value) { this->active_state_drying_value_ = value; }
  void set_active_state_fanonly_value(uint8_t value) { this->active_state_fanonly_value_ = value; }
  void set_heating_state_pin(GPIOPin *pin) { this->heating_state_pin_ = pin; }
  void set_cooling_state_pin(GPIOPin *pin) { this->cooling_state_pin_ = pin; }
  void set_swing_vertical_id(const MatchingDatapoint& swing_vertical_id) { this->swing_vertical_id_ = swing_vertical_id; }
  void set_swing_horizontal_id(const MatchingDatapoint& swing_horizontal_id) { this->swing_horizontal_id_ = swing_horizontal_id; }
  void set_fan_speed_id(const MatchingDatapoint& fan_speed_id) { this->fan_speed_id_ = fan_speed_id; }
  void set_fan_speed_low_value(uint8_t fan_speed_low_value) { this->fan_speed_low_value_ = fan_speed_low_value; }
  void set_fan_speed_medium_value(uint8_t fan_speed_medium_value) {
    this->fan_speed_medium_value_ = fan_speed_medium_value;
  }
  void set_fan_speed_middle_value(uint8_t fan_speed_middle_value) {
    this->fan_speed_middle_value_ = fan_speed_middle_value;
  }
  void set_fan_speed_high_value(uint8_t fan_speed_high_value) { this->fan_speed_high_value_ = fan_speed_high_value; }
  void set_fan_speed_auto_value(uint8_t fan_speed_auto_value) { this->fan_speed_auto_value_ = fan_speed_auto_value; }
  void set_target_temperature_id(MatchingDatapoint target_temperature_dp, const float offset = 0.0f, const float temperature_multiplier = 1.0f) {
    this->dp_target_temperature_.emplace([this](const float value){this->on_target_temperature_value(value);},
                          std::move(target_temperature_dp),
                          offset, temperature_multiplier);
  }
  void set_current_temperature_id(MatchingDatapoint current_temperature_dp, const float offset = 0.0f, const float temperature_multiplier = 1.0f) {
    this->dp_current_temperature_.emplace([this](const float value){this->on_current_temperature_value(value);},
                          std::move(current_temperature_dp),
                          offset, temperature_multiplier);
  }
  void set_eco_id(MatchingDatapoint eco_dp, const bool inverted = false) {
    this->dp_eco_.emplace([this](const bool value){this->on_eco_value(value);},
                          std::move(eco_dp),
                          inverted);
  }
  void set_eco_temperature(float eco_temperature) { this->eco_temperature_ = eco_temperature; }
  void set_sleep_id(MatchingDatapoint sleep_dp, const bool inverted = false) {
    this->dp_sleep_.emplace([this](const bool value){this->on_sleep_value(value);},
                             std::move(sleep_dp),
                             inverted);
  }

  void set_reports_fahrenheit() { this->reports_fahrenheit_ = true; }

  void set_uyat_parent(Uyat *parent) { this->parent_ = parent; }

 protected:
  /// Override control to change settings of the climate device.
  void control(const climate::ClimateCall &call) override;

  /// Override control to change settings of swing mode.
  void control_swing_mode_(const climate::ClimateCall &call);

  /// Override control to change settings of fan mode.
  void control_fan_mode_(const climate::ClimateCall &call);

  /// Return the traits of this controller.
  climate::ClimateTraits traits() override;

  /// Re-compute the active preset of this climate controller.
  void compute_preset_();

  /// Re-compute the target temperature of this climate controller.
  void compute_target_temperature_();

  /// Re-compute the state of this climate controller.
  void compute_state_();

  /// Re-Compute the swing mode of this climate controller.
  void compute_swingmode_();

  /// Re-Compute the fan mode of this climate controller.
  void compute_fanmode_();

  /// Switch the climate device to the given climate mode.
  void switch_to_action_(climate::ClimateAction action);

  Uyat *parent_;
  bool supports_heat_;
  bool supports_cool_;
  std::optional<DpSwitch> dp_switch_{};
  optional<MatchingDatapoint> active_state_id_{};
  optional<uint8_t> active_state_heating_value_{};
  optional<uint8_t> active_state_cooling_value_{};
  optional<uint8_t> active_state_drying_value_{};
  optional<uint8_t> active_state_fanonly_value_{};
  GPIOPin *heating_state_pin_{nullptr};
  GPIOPin *cooling_state_pin_{nullptr};
  std::optional<DpNumber> dp_target_temperature_{};
  std::optional<DpNumber> dp_current_temperature_{};
  float hysteresis_{1.0f};
  std::optional<DpSwitch> dp_eco_{};
  std::optional<DpSwitch> dp_sleep_{};
  optional<float> eco_temperature_{};
  uint8_t active_state_;
  uint8_t fan_state_;
  optional<MatchingDatapoint> swing_vertical_id_{};
  optional<MatchingDatapoint> swing_horizontal_id_{};
  optional<MatchingDatapoint> fan_speed_id_{};
  optional<uint8_t> fan_speed_low_value_{};
  optional<uint8_t> fan_speed_medium_value_{};
  optional<uint8_t> fan_speed_middle_value_{};
  optional<uint8_t> fan_speed_high_value_{};
  optional<uint8_t> fan_speed_auto_value_{};
  bool swing_vertical_{false};
  bool swing_horizontal_{false};
  bool heating_state_{false};
  bool cooling_state_{false};
  float manual_temperature_;
  bool eco_;
  bool sleep_;
  bool reports_fahrenheit_{false};
};

}  // namespace uyat
}  // namespace esphome
