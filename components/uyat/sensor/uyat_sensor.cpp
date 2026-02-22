#include "esphome/core/log.h"
#include "esphome/core/entity_base.h"
#include "uyat_sensor.h"

namespace esphome::uyat
{

UyatSensor::UyatSensor(Uyat *parent, Config config):
parent_(*parent),
dp_number_([this](const float value){this->on_value(value);},
            std::move(config.matching_dp),
            0, 1.0f)
{}

void UyatSensor::setup() {
  this->dp_number_.init(this->parent_);
}

void UyatSensor::dump_config() {
  LOG_SENSOR("", "Uyat Sensor", this);
  char config_str[UYAT_LOG_BUFFER_SIZE];
  this->dp_number_.get_config().to_string(config_str, sizeof(config_str));
  ESP_LOGCONFIG(UyatSensor::TAG, "  Sensor %s is %s", get_name().c_str(), config_str);
}

void UyatSensor::on_value(const float value)
{
  ESP_LOGV(UyatSensor::TAG, "MCU reported %s is: %.4f", get_name().c_str(), value);
  this->publish_state(value);
}

}  // namespace
