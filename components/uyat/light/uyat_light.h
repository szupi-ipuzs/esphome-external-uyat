#pragma once

#include "esphome/core/component.h"
#include "esphome/components/light/light_output.h"

#include "../uyat.h"
#include "../dp_number.h"
#include "../dp_switch.h"
#include "../dp_color.h"
#include "../dp_dimmer.h"

namespace esphome::uyat
{

class UyatLight : public EntityBase, public Component, public light::LightOutput {
 public:

  struct ConfigDimmer
  {
    MatchingDatapoint dimmer_dp;
    uint32_t min_value;
    uint32_t max_value;
    bool inverted;
    std::optional<MatchingDatapoint> min_value_dp;
  };

  struct ConfigSwitch
  {
    MatchingDatapoint switch_dp;
    bool inverted;
  };

  struct ConfigColor
  {
    MatchingDatapoint color_dp;
    UyatColorType color_type;
  };

  struct ConfigWhiteTemperature
  {
    MatchingDatapoint white_temperature_dp;
    uint32_t min_value;
    uint32_t max_value;
    bool inverted;
    float cold_white_temperature;
    float warm_white_temperature;
  };

  struct Config
  {
    std::optional<ConfigDimmer> dimmer_config;
    std::optional<ConfigSwitch> switch_config;
    std::optional<ConfigColor> color_config;
    std::optional<ConfigWhiteTemperature> wt_config;
    bool color_interlock;
  };

  explicit UyatLight(Uyat *parent, Config config);
  void setup() override;
  void dump_config() override;

  light::LightTraits get_traits() override;
  void setup_state(light::LightState *state) override;
  void write_state(light::LightState *state) override;

 private:

  static constexpr const char* TAG = "uyat.light";

  struct Dimmer
  {
    DpDimmer dimmer;
    std::optional<DpNumber> min_value_number;
  };

  void on_dimmer_value(const float);
  void on_white_temperature_value(const float);
  void on_switch_value(const bool);
  void on_color_value(const DpColor::Value&);

  void configure_dimmer(ConfigDimmer config);
  void configure_switch(ConfigSwitch config);
  void configure_color(ConfigColor config);
  void configure_white_temperature(ConfigWhiteTemperature config);

  Uyat& parent_;
  std::optional<Dimmer> dimmer_{};
  std::optional<DpSwitch> dp_switch_{};
  std::optional<DpColor> dp_color_{};
  std::optional<DpDimmer> dp_white_temperature_{};
  float cold_white_temperature_;
  float warm_white_temperature_;
  bool color_interlock_{false};
  light::LightState *state_{nullptr};
};

}  // namespace esphome::uyat
