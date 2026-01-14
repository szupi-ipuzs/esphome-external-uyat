#include "esphome/core/log.h"
#include "esphome/core/entity_base.h"
#include "uyat_binary_sensor.h"

namespace esphome {
namespace uyat {

static const char *const TAG = "uyat.binary_sensor";

void UyatBinarySensor::setup() {
  if (!this->parent_)
  {
     ESP_LOGE(TAG, "Uyat parent not set for %s", this->get_object_id().c_str());
     return;
  }
  this->dp_binary_sensor_->init(*(this->parent_));
}

void UyatBinarySensor::dump_config() {
  ESP_LOGCONFIG(TAG, "Uyat Binary Sensor:");
  ESP_LOGCONFIG(TAG, "  Binary Sensor %s is %s", get_object_id().c_str(), this->dp_binary_sensor_? this->dp_binary_sensor_->get_config().to_string().c_str() : "misconfigured!");
}

void UyatBinarySensor::on_value(const bool value)
{
  ESP_LOGV(TAG, "MCU reported %s is: %s", get_object_id().c_str(), ONOFF(value));
  this->publish_state(value);
}


}  // namespace uyat
}  // namespace esphome
