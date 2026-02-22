#include "esphome/core/log.h"
#include "esphome/core/entity_base.h"

#include "uyat_text_sensor.h"

namespace esphome {
namespace uyat {

UyatTextSensor::UyatTextSensor(Uyat *parent, Config config):
parent_(*parent),
dp_text_([this](const std::string& value){this->on_value(value);},
          std::move(config.matching_dp),
          config.encoding)
{}

void UyatTextSensor::setup() {
  this->dp_text_.init(this->parent_);
}

void UyatTextSensor::dump_config() {
  ESP_LOGCONFIG(UyatTextSensor::TAG, "Uyat Text Sensor:");
  char config_str[UYAT_LOG_BUFFER_SIZE];
  this->dp_text_.get_config().to_string(config_str, sizeof(config_str));
  ESP_LOGCONFIG(UyatTextSensor::TAG, "  Text Sensor %s is %s", get_name().c_str(), config_str);
}

void UyatTextSensor::on_value(const std::string& value)
{
  ESP_LOGV(UyatTextSensor::TAG, "MCU reported %s is: %s", get_name().c_str(), value.c_str());
  this->publish_state(value);
}

}  // namespace uyat
}  // namespace esphome
