#pragma once

#include "esphome/core/component.h"
#include "esphome/components/light/light_output.h"

#include "../uyat.h"
#include "../dp_number.h"
#include "../dp_switch.h"
#include "../dp_color.h"
#include "uyat_light_dimmer.h"

namespace esphome::uyat
{

class UyatLight : public Component, public light::LightOutput {
 private:
  void on_dimmer_value(const float);
  void on_switch_value(const bool);
  void on_color_value(const DpColor::Value&);

 public:
  void setup() override;
  void dump_config() override;
  void configure_dimmer(MatchingDatapoint dimmer_dp, const uint32_t min_value, const uint32_t max_value, std::optional<MatchingDatapoint> min_value_dp = std::nullopt) {
    this->dimmer_.emplace([this](const float brightness_percent){ this->on_dimmer_value(brightness_percent); },
                          std::move(dimmer_dp),
                          min_value, max_value,
                          min_value_dp
                        );
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
  void set_color_temperature_id(uint8_t color_temperature_id) { this->color_temperature_id_ = color_temperature_id; }
  void set_color_temperature_invert(bool color_temperature_invert) {
    this->color_temperature_invert_ = color_temperature_invert;
  }
  void set_uyat_parent(Uyat *parent) { this->parent_ = parent; }
  void set_color_temperature_max_value(uint32_t color_temperature_max_value) {
    this->color_temperature_max_value_ = color_temperature_max_value;
  }
  void set_cold_white_temperature(float cold_white_temperature) {
    this->cold_white_temperature_ = cold_white_temperature;
  }
  void set_warm_white_temperature(float warm_white_temperature) {
    this->warm_white_temperature_ = warm_white_temperature;
  }
  void set_color_interlock(bool color_interlock) { color_interlock_ = color_interlock; }

  light::LightTraits get_traits() override;
  void setup_state(light::LightState *state) override;
  void write_state(light::LightState *state) override;

 protected:

  Uyat *parent_;
  std::optional<UyatLightDimmer> dimmer_{};
  std::optional<DpSwitch> dp_switch_{};
  std::optional<DpColor> dp_color_{};
  optional<uint8_t> color_temperature_id_{};
  uint32_t color_temperature_max_value_ = 255;
  float cold_white_temperature_;
  float warm_white_temperature_;
  bool color_temperature_invert_{false};
  bool color_interlock_{false};
  light::LightState *state_{nullptr};
};

}  // namespace esphome::uyat
