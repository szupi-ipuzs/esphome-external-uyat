#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"

#include "../uyat.h"
#include "../dp_switch.h"
#include "../dp_number.h"

namespace esphome {
namespace uyat {

struct ActiveStateDpValueMapping
{
   std::optional<uint32_t> heating_value;
   std::optional<uint32_t> cooling_value;
   std::optional<uint32_t> drying_value;
   std::optional<uint32_t> fanonly_value;
};

class UyatClimate : public climate::Climate, public Component {
 private:

  static constexpr const char * TAG = "uyat.climate";

  void on_switch_value(const bool);
  void on_sleep_value(const bool);
  void on_eco_value(const bool);
  void on_boost_value(const bool);
  void on_target_temperature_value(const float);
  void on_current_temperature_value(const float);
  void on_active_state_value(const float);

 public:

  void setup() override;
  void loop() override;
  void dump_config() override;
  void set_supported_modes(const bool supports_heat, const bool supports_cool) {
    this->supports_heat_ = supports_heat;
    this->supports_cool_ = supports_cool;
  }
  void configure_switch(MatchingDatapoint switch_dp, const bool inverted = false) {
    this->dp_switch_.emplace([this](const bool value){this->on_switch_value(value);},
                             std::move(switch_dp),
                             inverted);
  }
  void configure_active_state_dp(MatchingDatapoint state_dp, const ActiveStateDpValueMapping& value_mapping)
  {
    this->dp_active_state_.emplace(ActiveStateDp{
                              DpNumber([this](const float value){this->on_active_state_value(value);},
                                       std::move(state_dp),
                                       0.0f, 1.0f
                                      ),
                              value_mapping
                            });
  }
  void set_heating_state_pin(GPIOPin *pin) { this->active_state_pins_.heating = pin; }
  void set_cooling_state_pin(GPIOPin *pin) { this->active_state_pins_.cooling = pin; }
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

  void configure_preset_boost(MatchingDatapoint boost_dp, const std::optional<float> boost_temperature, const bool inverted = false) {
    this->presets_.boost.dp.emplace([this](const bool value){this->on_boost_value(value);},
                             std::move(boost_dp),
                             inverted);
    this->presets_.boost.temperature = boost_temperature;
  }

  void configure_preset_eco(MatchingDatapoint eco_dp, const std::optional<float> eco_temperature, const bool inverted = false) {
    this->presets_.eco.dp.emplace([this](const bool value){this->on_eco_value(value);},
                          std::move(eco_dp),
                          inverted);
    this->presets_.eco.temperature = eco_temperature;
  }
  void configure_preset_sleep(MatchingDatapoint sleep_dp, const bool inverted = false) {
    this->presets_.sleep.dp.emplace([this](const bool value){this->on_sleep_value(value);},
                             std::move(sleep_dp),
                             inverted);
  }

  void set_reports_fahrenheit() { this->reports_fahrenheit_ = true; }

  void set_uyat_parent(Uyat *parent) { this->parent_ = parent; }

 protected:

  struct ActiveStateDp
  {
     DpNumber dp_number;
     ActiveStateDpValueMapping mapping;

     std::optional<climate::ClimateMode> last_value_to_mode() const
     {
       const auto last_value = dp_number.get_last_received_value();
       if (!last_value)
       {
         return std::nullopt;
       }

       if (mapping.heating_value == static_cast<uint32_t>(*last_value))
       {
         return climate::CLIMATE_MODE_HEAT;
       }
       if (mapping.cooling_value == static_cast<uint32_t>(*last_value))
       {
         return climate::CLIMATE_MODE_COOL;
       }
       if (mapping.drying_value == static_cast<uint32_t>(*last_value))
       {
         return climate::CLIMATE_MODE_DRY;
       }
       if (mapping.fanonly_value == static_cast<uint32_t>(*last_value))
       {
         return climate::CLIMATE_MODE_FAN_ONLY;
       }
       return std::nullopt;
     }

     std::optional<climate::ClimateAction> last_value_to_action() const
     {
       const auto last_value = dp_number.get_last_received_value();
       if (!last_value)
       {
         return std::nullopt;
       }

       if (mapping.heating_value == static_cast<uint32_t>(*last_value))
       {
         return climate::CLIMATE_ACTION_HEATING;
       }
       if (mapping.cooling_value == static_cast<uint32_t>(*last_value))
       {
         return climate::CLIMATE_ACTION_COOLING;
       }
       if (mapping.drying_value == static_cast<uint32_t>(*last_value))
       {
         return climate::CLIMATE_ACTION_DRYING;
       }
       if (mapping.fanonly_value == static_cast<uint32_t>(*last_value))
       {
         return climate::CLIMATE_ACTION_FAN;
       }
       return std::nullopt;
     }

     bool apply_mode(const climate::ClimateMode new_mode)
     {
        switch(new_mode)
        {
          case climate::CLIMATE_MODE_HEAT:
          {
            if (!mapping.heating_value.has_value())
            {
              return false;
            }

            dp_number.set_value(*mapping.heating_value);
            return true;
          }
          case climate::CLIMATE_MODE_COOL:
          {
            if (!mapping.cooling_value.has_value())
            {
              return false;
            }

            dp_number.set_value(*mapping.cooling_value);
            return true;
          }
          case climate::CLIMATE_MODE_DRY:
          {
            if (!mapping.drying_value.has_value())
            {
              return false;
            }

            dp_number.set_value(*mapping.drying_value);
            return true;
          }
          case climate::CLIMATE_MODE_FAN_ONLY:
          {
            if (!mapping.fanonly_value.has_value())
            {
              return false;
            }

            dp_number.set_value(*mapping.fanonly_value);
            return true;
          }
          default:
            return false;
        }
     }
  };

  struct ActiveStatePins
  {
    GPIOPin *heating{nullptr};
    GPIOPin *cooling{nullptr};

    bool heating_state{false};
    bool cooling_state{false};
  };

  struct AnyPreset
  {
    std::optional<DpSwitch> dp{};
    std::optional<float> temperature{};
  };

  struct Presets
  {
    AnyPreset boost{};
    AnyPreset eco{};
    AnyPreset sleep{};

    void init(DatapointHandler& dp_handler)
    {
      if (boost.dp.has_value()) {
        boost.dp->init(dp_handler);
      }
      if (eco.dp.has_value()) {
        eco.dp->init(dp_handler);
      }
      if (sleep.dp.has_value()) {
        sleep.dp->init(dp_handler);
      }
    }

    void dump_config() const
    {
      if (boost.dp.has_value()) {
        ESP_LOGCONFIG(UyatClimate::TAG, "  Boost is %s", boost.dp->get_config().to_string().c_str());
      }
      if (eco.dp.has_value()) {
        ESP_LOGCONFIG(UyatClimate::TAG, "  Eco is %s", eco.dp->get_config().to_string().c_str());
      }
      if (sleep.dp.has_value()) {
        ESP_LOGCONFIG(UyatClimate::TAG, "  Sleep is %s", sleep.dp->get_config().to_string().c_str());
      }
    }

    std::vector<esphome::climate::ClimatePreset> get_supported_presets() const
    {
      std::vector<esphome::climate::ClimatePreset> result;

      if (boost.dp)
      {
        result.push_back(climate::CLIMATE_PRESET_BOOST);
      }
      if (eco.dp)
      {
        result.push_back(climate::CLIMATE_PRESET_ECO);
      }
      if (sleep.dp)
      {
        result.push_back(climate::CLIMATE_PRESET_SLEEP);
      }

      return result;
    }

    esphome::climate::ClimatePreset get_active_preset() const
    {
      if (boost.dp && boost.dp->get_last_received_value().value_or(false))
      {
        return climate::CLIMATE_PRESET_BOOST;
      }
      if (eco.dp && eco.dp->get_last_received_value().value_or(false))
      {
        return climate::CLIMATE_PRESET_ECO;
      }
      if (sleep.dp && sleep.dp->get_last_received_value().value_or(false))
      {
        return climate::CLIMATE_PRESET_SLEEP;
      }

      return climate::CLIMATE_PRESET_NONE;
    }

    std::optional<float> get_active_preset_temperature() const
    {
      if (eco.dp && eco.dp->get_last_received_value().value_or(false))
      {
        return eco.temperature;
      }
      if (sleep.dp && sleep.dp->get_last_received_value().value_or(false))
      {
        return sleep.temperature;
      }

      return std::nullopt;
    }

    void apply_preset(const esphome::climate::ClimatePreset preset_to_apply)
    {
      ESP_LOGV(TAG, "Applying preset: %d", preset_to_apply);
      if (boost.dp.has_value()) {
        const bool boost_value = preset_to_apply == climate::CLIMATE_PRESET_BOOST;
        ESP_LOGV(TAG, "Setting boost: %s", ONOFF(boost_value));
        boost.dp->set_value(boost_value);
      }
      if (eco.dp.has_value()) {
        const bool eco_value = preset_to_apply == climate::CLIMATE_PRESET_ECO;
        ESP_LOGV(TAG, "Setting eco: %s", ONOFF(eco_value));
        eco.dp->set_value(eco_value);
      }
      if (sleep.dp.has_value()) {
        const bool sleep_value = preset_to_apply == climate::CLIMATE_PRESET_SLEEP;
        ESP_LOGV(TAG, "Setting sleep: %s", ONOFF(sleep_value));
        sleep.dp->set_value(sleep_value);
      }
    }
  };

  /// Override control to change settings of the climate device.
  void control(const climate::ClimateCall &call) override;

  /// Override control to change settings of swing mode.
  void control_swing_mode_(const climate::ClimateCall &call);

  /// Override control to change settings of fan mode.
  void control_fan_mode_(const climate::ClimateCall &call);

  /// Return the traits of this controller.
  climate::ClimateTraits traits() override;

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
  std::optional<ActiveStateDp> dp_active_state_{};
  ActiveStatePins active_state_pins_{};
  std::optional<DpNumber> dp_target_temperature_{};
  std::optional<DpNumber> dp_current_temperature_{};
  Presets presets_{};
  float hysteresis_{1.0f};
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
  float manual_temperature_;
  bool reports_fahrenheit_{false};
};

}  // namespace uyat
}  // namespace esphome
