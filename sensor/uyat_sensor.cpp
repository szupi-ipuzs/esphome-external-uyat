#include "esphome/core/log.h"
#include "esphome/core/entity_base.h"
#include "uyat_sensor.h"

namespace esphome::uyat
{

static const char *const TAG = "uyat.sensor";

void UyatSensor::setup() {
  assert(this->parent_);
  this->dp_number_->init(*(this->parent_));
}

void UyatSensor::dump_config() {
  LOG_SENSOR("", "Uyat Sensor", this);
  ESP_LOGCONFIG(TAG, "  Sensor %s is %s", get_object_id().c_str(), this->dp_number_? this->dp_number_->get_config().to_string().c_str() : "misconfigured!");
}

void UyatSensor::on_value(const float value)
{
  ESP_LOGV(TAG, "MCU reported %s is: %.4f", get_object_id().c_str(), value);
  this->publish_state(value);
}

}  // namespace
