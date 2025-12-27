#include "esphome/core/log.h"
#include "uyat_binary_sensor.h"

namespace esphome {
namespace uyat {

static const char *const TAG = "uyat.binary_sensor";

void UyatBinarySensor::setup() {
  this->parent_->register_listener(this->sensor_id_, [this](const UyatDatapoint &datapoint) {
    auto * dp_value = std::get_if<BoolDatapointValue>(&datapoint.value);
    if (!dp_value)
    {
      ESP_LOGW(TAG, "Unexpected datapoint type!");
      return;
    }

    ESP_LOGV(TAG, "MCU reported binary sensor %u is: %s", datapoint.number, ONOFF(dp_value->value));
    this->publish_state(dp_value->value);
  });
}

void UyatBinarySensor::dump_config() {
  ESP_LOGCONFIG(TAG, "Uyat Binary Sensor:");
  ESP_LOGCONFIG(TAG, "  Binary Sensor has datapoint ID %u", this->sensor_id_);
}

}  // namespace uyat
}  // namespace esphome
