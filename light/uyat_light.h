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

class UyatLight : public Component, public light::LightOutput {
 private:
  void on_dimmer_value(const float);
  void on_white_temperature_value(const float);
  void on_switch_value(const bool);
  void on_color_value(const DpColor::Value&);

 public:
  void setup() override;
  void dump_config() override;
  void configure_dimmer(MatchingDatapoint dimmer_dp, const uint32_t min_value, const uint32_t max_value, const bool inverted, std::optional<MatchingDatapoint> min_value_dp = std::nullopt) {
    this->dimmer_.emplace(
                          DpDimmer{
                            [this](const float brightness_percent){ this->on_dimmer_value(brightness_percent); },
                            std::move(dimmer_dp),
                            min_value, max_value,
                            inverted
                          },
                          std::nullopt
                         );
    if (min_value_dp)
    {
      this->dimmer_->min_value_number.emplace(
        [](auto){}, // ignore (write only)
        std::move(*min_value_dp),
        0.0f, 1.0f
      );
    }
  }

  void configure_switch(MatchingDatapoint dimmer_dp, const bool inverted) {
    this->dp_switch_.emplace([this](const bool value){ this->on_switch_value(value);},
                             std::move(dimmer_dp),
                             inverted);
  }
  void configure_color(MatchingDatapoint color_dp, const UyatColorType color_type){
    this->dp_color_.emplace([this](const DpColor::Value& value){ this->on_color_value(value);},
                             std::move(color_dp),
                             color_type);
  }
  void configure_white_temperature(MatchingDatapoint dimmer_dp,
                                   const uint32_t min_value,
                                   const uint32_t max_value,
                                   const bool inverted,
                                   float cold_white_temperature,
                                   float warm_white_temperature) {
    this->dp_white_temperature_.emplace([this](const float brightness_percent){ this->on_white_temperature_value(brightness_percent); },
                          std::move(dimmer_dp),
                          min_value, max_value,
                          inverted
                        );
    this->cold_white_temperature_ = cold_white_temperature;
    this->warm_white_temperature_ = warm_white_temperature;
  }

  void set_uyat_parent(Uyat *parent) { this->parent_ = parent; }

  void set_color_interlock(bool color_interlock) { this->color_interlock_ = color_interlock; }

  light::LightTraits get_traits() override;
  void setup_state(light::LightState *state) override;
  void write_state(light::LightState *state) override;

 protected:

  struct DimmerConfig
  {
    DpDimmer dimmer;
    std::optional<DpNumber> min_value_number;
  };

  Uyat *parent_;
  std::optional<DimmerConfig> dimmer_{};
  std::optional<DpSwitch> dp_switch_{};
  std::optional<DpColor> dp_color_{};
  std::optional<DpDimmer> dp_white_temperature_{};
  float cold_white_temperature_;
  float warm_white_temperature_;
  bool color_interlock_{false};
  light::LightState *state_{nullptr};
};

}  // namespace esphome::uyat
