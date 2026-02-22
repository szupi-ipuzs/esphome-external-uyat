#include "esphome/core/log.h"
#include "esphome/core/entity_base.h"
#include "uyat_binary_sensor.h"

namespace esphome {
namespace uyat {

UyatBinarySensor::UyatBinarySensor(Uyat* parent, Config config):
parent_(*parent),
dp_binary_sensor_([this](const bool value){this->on_value(value);},
                  std::move(config.sensor_dp),
                  config.bit_number,
                   // note: we could let DpBinarySensor handle invertions,
                   // but with the binary_sensor has its own way to handle this using filters
                  false)
{}

void UyatBinarySensor::setup() {
  this->dp_binary_sensor_.init(this->parent_);
}

void UyatBinarySensor::dump_config() {
  ESP_LOGCONFIG(UyatBinarySensor::TAG, "Uyat Binary Sensor:");
  char config_str[UYAT_LOG_BUFFER_SIZE];
  this->dp_binary_sensor_.get_config().to_string(config_str, sizeof(config_str));
  ESP_LOGCONFIG(UyatBinarySensor::TAG, "  Binary Sensor %s is %s", get_name().c_str(), config_str);
}

void UyatBinarySensor::on_value(const bool value)
{
  ESP_LOGV(UyatBinarySensor::TAG, "MCU reported %s is: %s", get_name().c_str(), ONOFF(value));
  this->publish_state(value);
}


}  // namespace uyat
}  // namespace esphome
