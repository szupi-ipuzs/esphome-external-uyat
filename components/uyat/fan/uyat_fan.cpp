#include "esphome/core/log.h"
#include "uyat_fan.h"

namespace esphome {
namespace uyat {

UyatFan::UyatFan(Uyat *parent, Config config):
parent_(*parent)
{
  if (config.speed_config)
  {
    configure_speed(std::move(*config.speed_config));
  }
  if (config.switch_config)
  {
    configure_switch(std::move(*config.switch_config));
  }
  if (config.oscillation_config)
  {
    configure_oscillation(std::move(*config.oscillation_config));
  }
  if (config.direction_config)
  {
    configure_direction(std::move(*config.direction_config));
  }
}


void UyatFan::setup() {
  if (this->speed_.has_value()) {
    this->speed_->dp_speed.init(this->parent_);
  }
  if (this->dp_switch_.has_value()) {
    this->dp_switch_->init(this->parent_);
  }
  if (this->dp_oscillation_.has_value()) {
    this->dp_oscillation_->init(this->parent_);
  }
  if (this->dp_direction_.has_value()) {
    this->dp_direction_->init(this->parent_);
  }

  this->parent_.add_on_initialized_callback([this]() {
    auto restored = this->restore_state_();
    if (restored)
      restored->to_call(*this).perform();
  });
}

void UyatFan::dump_config() {
  LOG_FAN("", "Uyat Fan", this);
  if (this->speed_.has_value()) {
    ESP_LOGCONFIG(UyatFan::TAG, "  Speed is %s, [%u, %u]", this->speed_->dp_speed.get_config().to_string().c_str(), this->speed_->min_value, this->speed_->max_value);
  }
  if (this->dp_switch_.has_value()) {
    ESP_LOGCONFIG(UyatFan::TAG, "  Switch is %s", this->dp_switch_->get_config().to_string().c_str());
  }
  if (this->dp_oscillation_.has_value()) {
    ESP_LOGCONFIG(UyatFan::TAG, "  Oscillation is %s", this->dp_oscillation_->get_config().to_string().c_str());
  }
  if (this->dp_direction_.has_value()) {
    ESP_LOGCONFIG(UyatFan::TAG, "  Direction is %s", this->dp_direction_->get_config().to_string().c_str());
  }
}

fan::FanTraits UyatFan::get_traits() {
  int speed_count = 0;
  if (this->speed_)
  {
    speed_count = this->speed_->get_number_of_speeds();
  }
  return fan::FanTraits(this->dp_oscillation_.has_value(), this->speed_.has_value(), this->dp_direction_.has_value(),
                        speed_count);
}

void UyatFan::control(const fan::FanCall &call) {
  if (this->dp_switch_.has_value() && call.get_state().has_value()) {
    this->dp_switch_->set_value(*call.get_state());
  }
  if (this->dp_oscillation_.has_value() && call.get_oscillating().has_value()) {
    this->dp_oscillation_->set_value(*call.get_oscillating());
  }
  if (this->dp_direction_.has_value() && call.get_direction().has_value()) {
    this->dp_direction_->set_value(*call.get_direction() == fan::FanDirection::FORWARD);
  }
  if (this->speed_.has_value() && call.get_speed().has_value()) {
    auto value_to_set = *call.get_speed() + this->speed_->min_value - 1; // get_speed() returns a 1-based index
    if (value_to_set > this->speed_->max_value)
    {
      value_to_set = this->speed_->max_value;
    }
    this->speed_->dp_speed.set_value(value_to_set);
  }
}

void UyatFan::on_switch_value(const bool value)
{
  ESP_LOGV(UyatFan::TAG, "MCU reported switch %s is: %s", get_name().c_str(), ONOFF(value));
  this->state = value;
  this->publish_state();
}

void UyatFan::on_oscillation_value(const bool value)
{
  ESP_LOGV(UyatFan::TAG, "MCU reported oscillation is: %s", ONOFF(value));

  this->oscillating = value;
  this->publish_state();
}

void UyatFan::on_direction_value(const bool value)
{
  ESP_LOGV(UyatFan::TAG, "MCU reported direction is: %s", ONOFF(value));

  this->direction = value ? fan::FanDirection::FORWARD : fan::FanDirection::REVERSE;
  this->publish_state();
}

void UyatFan::on_speed_value(const float value)
{
  ESP_LOGV(UyatFan::TAG, "MCU reported speed of: %.0f", value);

  this->speed = 1;
  if (value < this->speed_->min_value)
  {
    ESP_LOGW(UyatFan::TAG, "Value %.0f reported by MCU is out of range [%u, %u]", value, this->speed_->min_value, this->speed_->max_value);
  }
  else
  if (value > this->speed_->min_value)
  {
    ESP_LOGW(UyatFan::TAG, "Value %.0f reported by MCU is out of range [%u, %u]", value, this->speed_->min_value, this->speed_->max_value);
    this->speed = this->speed_->get_number_of_speeds();
  }
  else
  {
    this->speed = static_cast<int>(value) - this->speed_->min_value + 1;
  }

  this->publish_state();
}

void UyatFan::configure_speed(SpeedConfig&& config)
{
  this->speed_.emplace(
    DpNumber{
      [this](const float value){ this->on_speed_value(value); },
      std::move(config.matching_dp),
      0.0f, 1.0f
    },
    config.min_value,
    config.max_value
  );
}

void UyatFan::configure_switch(SwitchConfig&& config)
{
  this->dp_switch_.emplace(
    [this](const bool value){ this->on_switch_value(value); },
    std::move(config.matching_dp),
    config.inverted
  );
}

void UyatFan::configure_oscillation(OscillationConfig&& config)
{
  this->dp_oscillation_.emplace(
    [this](const bool value){ this->on_oscillation_value(value); },
    std::move(config.matching_dp),
    config.inverted
  );
}

void UyatFan::configure_direction(DirectionConfig&& config)
{
  this->dp_direction_.emplace(
    [this](const bool value){ this->on_direction_value(value); },
    std::move(config.matching_dp),
    config.inverted
  );
}

}  // namespace uyat
}  // namespace esphome
