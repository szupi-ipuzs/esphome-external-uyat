#include "esphome/core/log.h"
#include "esphome/core/entity_base.h"

#include "uyat_text_sensor.h"

namespace esphome {
namespace uyat {

UyatTextSensor::UyatTextSensor(Uyat *parent, Config config):
parent_(*parent),
dp_text_([this](const String& value){this->on_value(value);},
          std::move(config.matching_dp),
          config.encoding)
{}

void UyatTextSensor::setup() {
  this->dp_text_.init(this->parent_);
}

void UyatTextSensor::dump_config() {
  ESP_LOGCONFIG(UyatTextSensor::TAG, "Uyat Text Sensor:");
  ESP_LOGCONFIG(UyatTextSensor::TAG, "  Text Sensor %s is %s", get_name().c_str(), this->dp_text_.get_config().to_string().c_str());
}

void UyatTextSensor::on_value(const String& value)
{
  ESP_LOGV(UyatTextSensor::TAG, "MCU reported %s is: %s", get_name().c_str(), value.c_str());
  this->publish_state(value);
}

}  // namespace uyat
}  // namespace esphome
