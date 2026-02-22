#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"

#include "../uyat.h"
#include "../dp_switch.h"
#include "../dp_number.h"

namespace esphome {
namespace uyat {

class UyatClimate : public climate::Climate, public Component {
 public:

  struct SwitchConfig
  {
    MatchingDatapoint matching_dp;
    bool inverted;
  };

  struct AnySwingConfig
  {
    MatchingDatapoint matching_dp;
    bool inverted;
  };

  struct SwingsConfig
  {
    std::optional<AnySwingConfig> vertical;
    std::optional<AnySwingConfig> horizontal;
  };

  struct ActiveStatePinsConfig
  {
    GPIOPin *heating{nullptr};
    GPIOPin *cooling{nullptr};
  };


  struct ActiveStateDpValueMapping
  {
    std::optional<uint32_t> heating_value;
    std::optional<uint32_t> cooling_value;
    std::optional<uint32_t> drying_value;
    std::optional<uint32_t> fanonly_value;
  };

  struct ActiveStateDpConfig
  {
    MatchingDatapoint matching_dp;
    ActiveStateDpValueMapping mapping;
  };

  struct SinglePresetConfig
  {
    MatchingDatapoint matching_dp;
    bool inverted;
    std::optional<float> temperature;
  };

  struct PresetsConfig
  {
    std::optional<SinglePresetConfig> boost_config;
    std::optional<SinglePresetConfig> eco_config;
    std::optional<SinglePresetConfig> sleep_config;
  };


  struct FanSpeedDpValueMapping
  {
    std::optional<uint32_t> auto_value;
    std::optional<uint32_t> low_value;
    std::optional<uint32_t> medium_value;
    std::optional<uint32_t> middle_value;
    std::optional<uint32_t> high_value;
  };

  struct TemperatureDpConfig
  {
    MatchingDatapoint matching_dp;
    float offset;
    float multiplier;
  };

  struct TemperatureConfig
  {
    TemperatureDpConfig target_temperature;
    std::optional<TemperatureDpConfig> current_temperature;
    float hysteresis;
    bool reports_fahrenheit;
  };

  struct FanConfig
  {
    MatchingDatapoint matching_dp;
    FanSpeedDpValueMapping mapping;
  };

  struct Config
  {
    bool supports_heat;
    bool supports_cool;
    std::optional<SwitchConfig> switch_config;
    std::optional<ActiveStatePinsConfig> active_state_pins_config;
    std::optional<ActiveStateDpConfig> active_state_dp_config;
    std::optional<TemperatureConfig> temperature_config;
    std::optional<PresetsConfig> presets_config;
    std::optional<SwingsConfig> swings_config;
    std::optional<FanConfig> fan_config;
  };

  explicit UyatClimate(Uyat *parent, Config config);

  void setup() override;
  void loop() override;
  void dump_config() override;

 private:

  static constexpr const char * TAG = "uyat.climate";

  struct ActiveStateDpHandler
  {
     DpNumber dp_number;
     ActiveStateDpValueMapping mapping;

     void dump_config() const
     {
        char config_str[UYAT_LOG_BUFFER_SIZE];
        dp_number.get_config().to_string(config_str, sizeof(config_str));
        ESP_LOGCONFIG(UyatClimate::TAG, "  Active state is %s", config_str);
     }

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

  struct ActiveStatePinsHandler
  {
    void init()
    {
      if (heating != nullptr) {
        heating->setup();
        heating_state = heating->digital_read();
      }
      if (cooling != nullptr) {
        cooling->setup();
        cooling_state = cooling->digital_read();
      }
    }

    bool update_pins_state()
    {
      bool state_changed = false;
      if (heating != nullptr) {
        const auto new_heating_state = heating->digital_read();
        if (heating_state != new_heating_state) {
          ESP_LOGV(UyatClimate::TAG, "Heating state pin changed to: %s", ONOFF(new_heating_state));
          heating_state = new_heating_state;
          state_changed = true;
        }
      }
      if (cooling != nullptr) {
        bool new_cooling_state = cooling->digital_read();
        if (cooling_state != new_cooling_state) {
          ESP_LOGV(UyatClimate::TAG, "Cooling state pin changed to: %s", ONOFF(new_cooling_state));
          cooling_state = new_cooling_state;
          state_changed = true;
        }
      }

      return state_changed;
    }

    std::optional<climate::ClimateMode> mode_from_state() const
    {
      if ((heating == nullptr) && (cooling == nullptr))
      {
        return std::nullopt;
      }

      if (heating_state) {
        return climate::CLIMATE_MODE_HEAT;
      }

      if (cooling_state) {
        return climate::CLIMATE_MODE_COOL;
      }

      return climate::CLIMATE_MODE_OFF;
    }

    void dump_config() const
    {
      LOG_PIN("  Heating State Pin: ", heating);
      LOG_PIN("  Cooling State Pin: ", cooling);
    }

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

  struct PresetsHandler
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
      char config_str[UYAT_LOG_BUFFER_SIZE];
      
      if (boost.dp.has_value()) {
        boost.dp->get_config().to_string(config_str, sizeof(config_str));
        ESP_LOGCONFIG(UyatClimate::TAG, "  Boost is %s", config_str);
      }
      
      if (eco.dp.has_value()) {
        eco.dp->get_config().to_string(config_str, sizeof(config_str));
        ESP_LOGCONFIG(UyatClimate::TAG, "  Eco is %s", config_str);
      }
      
      if (sleep.dp.has_value()) {
        sleep.dp->get_config().to_string(config_str, sizeof(config_str));
        ESP_LOGCONFIG(UyatClimate::TAG, "  Sleep is %s", config_str);
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

  struct TemperaturesHandler
  {
    DpNumber dp_target;
    std::optional<DpNumber> dp_current;
    float hysteresis{1.0f};
    bool reports_fahrenheit{false};

    void dump_config() const
    {
      char config_str[UYAT_LOG_BUFFER_SIZE];
      
      dp_target.get_config().to_string(config_str, sizeof(config_str));
      ESP_LOGCONFIG(UyatClimate::TAG, "  Target Temperature is %s", config_str);
      
      if (dp_current.has_value()) {
        dp_current->get_config().to_string(config_str, sizeof(config_str));
        ESP_LOGCONFIG(UyatClimate::TAG, "  Current Temperature is %s", config_str);
      }
    }

    std::optional<float> get_current_temperature() const
    {
      if (!dp_current)
      {
        return std::nullopt;
      }
      const auto last_value = dp_current->get_last_received_value();
      if (!last_value)
      {
        return std::nullopt;
      }

      if (!reports_fahrenheit)
      {
        return last_value;
      }

      return (last_value.value() - 32) * 5 / 9;
    }

    std::optional<float> get_target_temperature() const
    {
      const auto last_value = dp_target.get_last_received_value();
      if (!last_value)
      {
        return std::nullopt;
      }

      if (!reports_fahrenheit)
      {
        return last_value;
      }

      return (last_value.value() - 32) * 5 / 9;
    }

    void set_target_temperature(float value)
    {
      if (reports_fahrenheit)
      {
        value = (value * 9 / 5) + 32;
      }

      ESP_LOGV(UyatClimate::TAG, "Setting target temperature: %.1f", value);
      dp_target.set_value(value);
    }
  };

  struct FanModesHandler
  {
    DpNumber dp_number;
    FanSpeedDpValueMapping mapping;

    std::optional<esphome::climate::ClimateFanMode> get_current_mode() const
    {
      const auto last_value = dp_number.get_last_received_value();
      if (!last_value)
      {
        return std::nullopt;
      }

      if (last_value == mapping.auto_value)
      {
        return climate::CLIMATE_FAN_AUTO;
      }

      if (last_value == mapping.high_value)
      {
        return climate::CLIMATE_FAN_HIGH;
      }

      if (last_value == mapping.medium_value)
      {
        return climate::CLIMATE_FAN_MEDIUM;
      }

      if (last_value == mapping.middle_value)
      {
        return climate::CLIMATE_FAN_MIDDLE;
      }

      if (last_value == mapping.low_value)
      {
        return climate::CLIMATE_FAN_LOW;
      }

      return std::nullopt;
    }

    std::vector<esphome::climate::ClimateFanMode> get_supported_modes() const
    {
      std::vector<esphome::climate::ClimateFanMode> result;

      if (mapping.low_value)
        result.push_back(climate::CLIMATE_FAN_LOW);
      if (mapping.medium_value)
        result.push_back(climate::CLIMATE_FAN_MEDIUM);
      if (mapping.middle_value)
        result.push_back(climate::CLIMATE_FAN_MIDDLE);
      if (mapping.high_value)
        result.push_back(climate::CLIMATE_FAN_HIGH);
      if (mapping.auto_value)
        result.push_back(climate::CLIMATE_FAN_AUTO);

      return result;
    }

    bool apply_mode(const esphome::climate::ClimateFanMode new_mode)
    {
      switch(new_mode)
      {
        case esphome::climate::CLIMATE_FAN_LOW:
        {
          if (!mapping.low_value)
          {
            return false;
          }

          dp_number.set_value(*mapping.low_value);
          return true;
        }
        case esphome::climate::CLIMATE_FAN_MEDIUM:
        {
          if (!mapping.medium_value)
          {
            return false;
          }

          dp_number.set_value(*mapping.medium_value);
          return true;
        }
        case esphome::climate::CLIMATE_FAN_MIDDLE:
        {
          if (!mapping.middle_value)
          {
            return false;
          }

          dp_number.set_value(*mapping.middle_value);
          return true;
        }
        case esphome::climate::CLIMATE_FAN_HIGH:
        {
          if (!mapping.high_value)
          {
            return false;
          }

          dp_number.set_value(*mapping.high_value);
          return true;
        }
        case esphome::climate::CLIMATE_FAN_AUTO:
        {
          if (!mapping.auto_value)
          {
            return false;
          }

          dp_number.set_value(*mapping.auto_value);
          return true;
        }
        default:
          return false;
      }
    }
  };

  struct SwingModesHandler
  {
    std::optional<DpSwitch> dp_vertical{};
    std::optional<DpSwitch> dp_horizontal{};

    void init(DatapointHandler& dp_handler)
    {
      if (dp_vertical.has_value()) {
        dp_vertical->init(dp_handler);
      }

      if (dp_horizontal.has_value()) {
        dp_horizontal->init(dp_handler);
      }
    }

    std::vector<esphome::climate::ClimateSwingMode> get_supported_swing_modes() const
    {
      if (dp_vertical.has_value())
      {
        if (dp_horizontal.has_value())
        {
          return {climate::CLIMATE_SWING_BOTH, climate::CLIMATE_SWING_VERTICAL, climate::CLIMATE_SWING_HORIZONTAL};
        }
        else
        {
          return {climate::CLIMATE_SWING_VERTICAL};
        }
      }
      else
      {
        if (dp_horizontal.has_value())
        {
          return {climate::CLIMATE_SWING_HORIZONTAL};
        }
        else
        {
          return {};
        }
      }
    }

    esphome::climate::ClimateSwingMode get_current_swing_mode() const
    {
      const bool swing_vertical = dp_vertical.has_value() && dp_vertical->get_last_received_value().value_or(false);
      const bool swing_horizontal = dp_horizontal.has_value() && dp_horizontal->get_last_received_value().value_or(false);

      if (swing_vertical && swing_horizontal) {
        return climate::CLIMATE_SWING_BOTH;
      } else if (swing_vertical) {
        return climate::CLIMATE_SWING_VERTICAL;
      } else if (swing_horizontal) {
        return climate::CLIMATE_SWING_HORIZONTAL;
      } else {
        return climate::CLIMATE_SWING_OFF;
      }
    }

    bool apply_swing_mode(const esphome::climate::ClimateSwingMode new_mode)
    {
      switch(new_mode)
      {
        case climate::CLIMATE_SWING_OFF:
        {
          if (dp_vertical.has_value())
          {
            dp_vertical->set_value(false);
          }
          if (dp_horizontal.has_value())
          {
            dp_horizontal->set_value(false);
          }

          return true;
        }
        case climate::CLIMATE_SWING_BOTH:
        {
          if (dp_vertical.has_value())
          {
            dp_vertical->set_value(true);
          }
          if (dp_horizontal.has_value())
          {
            dp_horizontal->set_value(true);
          }

          return (dp_vertical.has_value() && dp_horizontal.has_value());
        }
        case climate::CLIMATE_SWING_VERTICAL:
        {
          if (dp_vertical.has_value())
          {
            dp_vertical->set_value(true);
          }
          if (dp_horizontal.has_value())
          {
            dp_horizontal->set_value(false);
          }

          return (dp_vertical.has_value());
        }
        case climate::CLIMATE_SWING_HORIZONTAL:
        {
          if (dp_vertical.has_value())
          {
            dp_vertical->set_value(false);
          }
          if (dp_horizontal.has_value())
          {
            dp_horizontal->set_value(true);
          }

          return (dp_horizontal.has_value());
        }
        default:
          return false;
      }
    }
  };

  void configure_switch(SwitchConfig config) {
    this->dp_switch_.emplace([this](const bool value){this->on_switch_value(value);},
                             std::move(config.matching_dp),
                             config.inverted);
  }
  void configure_active_state_dp(ActiveStateDpConfig config)
  {
    this->dp_active_state_.emplace(ActiveStateDpHandler{
                              DpNumber([this](const float value){this->on_active_state_value(value);},
                                       std::move(config.matching_dp),
                                       0.0f, 1.0f
                                      ),
                              config.mapping
                            });
  }
  void configure_swings(SwingsConfig config){
    if (config.horizontal)
    {
      this->swing_modes_.dp_horizontal.emplace([this](const bool value){this->on_horizontal_swing(value);},
                                              std::move(config.horizontal->matching_dp),
                                              config.horizontal->inverted);
    }

    if (config.vertical)
    {
      this->swing_modes_.dp_vertical.emplace([this](const bool value){this->on_vertical_swing(value);},
                                              std::move(config.vertical->matching_dp),
                                              config.vertical->inverted);
    }
  }

  void configure_fan(FanConfig config){
    this->fan_modes_.emplace(
      FanModesHandler{
        DpNumber([this](const float value){this->on_fan_modes_value(value); },
          std::move(config.matching_dp),
          0.0f, 1.0f),
        config.mapping
      }
    );
  }

  void configure_temperatures(TemperatureConfig config) {
    this->temperatures_.emplace(TemperaturesHandler{
        DpNumber(
          [this](const float value){this->on_target_temperature_value(value);},
          std::move(config.target_temperature.matching_dp),
          config.target_temperature.offset, config.target_temperature.multiplier
        ),
        std::nullopt,
        config.hysteresis, config.reports_fahrenheit
      }
    );

    if (config.current_temperature)
    {
      this->temperatures_->dp_current.emplace(
          [this](const float value){this->on_current_temperature_value(value);},
          std::move(config.current_temperature->matching_dp),
          config.current_temperature->offset, config.current_temperature->multiplier
      );
    }
  }

  void configure_presets(PresetsConfig config)
  {
    if (config.boost_config)
    {
      this->presets_.boost.dp.emplace([this](const bool value){this->on_boost_value(value);},
                              std::move(config.boost_config->matching_dp),
                              config.boost_config->inverted);
      this->presets_.boost.temperature = config.boost_config->temperature;
    }
    if (config.eco_config)
    {
      this->presets_.eco.dp.emplace([this](const bool value){this->on_eco_value(value);},
                            std::move(config.eco_config->matching_dp),
                            config.eco_config->inverted);
      this->presets_.eco.temperature = config.eco_config->temperature;
    }
    if (config.sleep_config)
    {
      this->presets_.sleep.dp.emplace([this](const bool value){this->on_sleep_value(value);},
                              std::move(config.sleep_config->matching_dp),
                              config.sleep_config->inverted);
      this->presets_.sleep.temperature = config.sleep_config->temperature;
    }
  }


  void on_switch_value(const bool);
  void on_sleep_value(const bool);
  void on_eco_value(const bool);
  void on_boost_value(const bool);
  void on_target_temperature_value(const float);
  void on_current_temperature_value(const float);
  void on_active_state_value(const float);
  void on_fan_modes_value(const float);
  void on_horizontal_swing(const bool);
  void on_vertical_swing(const bool);

  /// Override control to change settings of the climate device.
  void control(const climate::ClimateCall &call) override;

  /// Override control to change settings of swing mode.
  void control_swing_mode_(const climate::ClimateCall &call);

  /// Override control to change settings of fan mode.
  void control_fan_mode_(const climate::ClimateCall &call);

  /// Return the traits of this controller.
  climate::ClimateTraits traits() override;

  /// Re-compute the target temperature of this climate controller.
  void select_target_temperature_to_report_();

  /// Re-compute the state of this climate controller.
  void compute_state_();

  /// Re-Compute the swing mode of this climate controller.
  void compute_swingmode_();

  /// Re-Compute the fan mode of this climate controller.
  void compute_fanmode_();

  /// Switch the climate device to the given climate mode.
  void switch_to_action_(climate::ClimateAction action);

  Uyat& parent_;
  bool supports_heat_;
  bool supports_cool_;
  std::optional<DpSwitch> dp_switch_{};
  std::optional<ActiveStateDpHandler> dp_active_state_{};
  ActiveStatePinsHandler active_state_pins_{};
  PresetsHandler presets_{};
  std::optional<TemperaturesHandler> temperatures_{};
  std::optional<FanModesHandler> fan_modes_{};
  SwingModesHandler swing_modes_{};
};

}  // namespace uyat
}  // namespace esphome
