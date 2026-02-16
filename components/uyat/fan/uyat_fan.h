#pragma once

#include "esphome/core/component.h"
#include "esphome/components/fan/fan.h"

#include "../uyat.h"
#include "../dp_switch.h"
#include "../dp_number.h"

namespace esphome {
namespace uyat {

class UyatFan : public Component, public fan::Fan {
 public:

  struct SpeedConfig
  {
    MatchingDatapoint matching_dp;
    uint32_t min_value;
    uint32_t max_value;
  };

  struct SwitchConfig
  {
    MatchingDatapoint matching_dp;
    bool inverted;
  };

  struct OscillationConfig
  {
    MatchingDatapoint matching_dp;
    bool inverted;
  };

  struct DirectionConfig
  {
    MatchingDatapoint matching_dp;
    bool inverted;
  };

  struct Config
  {
    std::optional<SpeedConfig> speed_config;
    std::optional<SwitchConfig> switch_config;
    std::optional<OscillationConfig> oscillation_config;
    std::optional<DirectionConfig> direction_config;
  };


  explicit UyatFan(Uyat *parent, Config config);
  void setup() override;
  void dump_config() override;

  fan::FanTraits get_traits() override;

 private:

  static constexpr const char* TAG = "uyat.fan";

  struct SpeedHandler
  {
    DpNumber dp_speed;
    uint32_t min_value;
    uint32_t max_value;

    int get_number_of_speeds() const
    {
      return max_value - min_value + 1;
    }
  };

  void control(const fan::FanCall &call) override;

  void configure_speed(SpeedConfig&& config);
  void configure_switch(SwitchConfig&& config);
  void configure_oscillation(OscillationConfig&& config);
  void configure_direction(DirectionConfig&& config);

  void on_speed_value(const float);
  void on_switch_value(const bool);
  void on_oscillation_value(const bool);
  void on_direction_value(const bool);


  Uyat& parent_;

  std::optional<SpeedHandler> speed_{};
  std::optional<DpSwitch> dp_switch_{};
  std::optional<DpSwitch> dp_oscillation_{};
  std::optional<DpSwitch> dp_direction_{};
};

}  // namespace uyat
}  // namespace esphome
