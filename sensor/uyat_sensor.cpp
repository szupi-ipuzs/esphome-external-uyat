#include "esphome/core/log.h"
#include "uyat_sensor.h"
#include <cinttypes>

namespace esphome {
namespace uyat {

static const char *const TAG = "uyat.sensor";

void UyatSensor::setup() {
  this->parent_->register_datapoint_listener(this->sensor_id_, [this](const UyatDatapoint &datapoint) {
    if (auto * dp_value = std::get_if<BoolDatapointValue>(&datapoint.value)) {
      ESP_LOGV(TAG, "MCU reported sensor %u is: %s", datapoint.number, ONOFF(dp_value->value));
      this->publish_state(dp_value->value);
    } else if (auto * dp_value = std::get_if<UIntDatapointValue>(&datapoint.value)) {
      ESP_LOGV(TAG, "MCU reported sensor %u is: %d", datapoint.number, dp_value->value);
      this->publish_state(dp_value->value);
    } else if (auto * dp_value = std::get_if<EnumDatapointValue>(&datapoint.value)) {
      ESP_LOGV(TAG, "MCU reported sensor %u is: %u", datapoint.number, dp_value->value);
      this->publish_state(dp_value->value);
    } else if (auto * dp_value = std::get_if<Bitmask8DatapointValue>(&datapoint.value)) {
      ESP_LOGV(TAG, "MCU reported sensor %u is: %" PRIx32, datapoint.number, dp_value->value);
      this->publish_state(dp_value->value);
    } else if (auto * dp_value = std::get_if<Bitmask16DatapointValue>(&datapoint.value)) {
      ESP_LOGV(TAG, "MCU reported sensor %u is: %" PRIx32, datapoint.number, dp_value->value);
      this->publish_state(dp_value->value);
    } else if (auto * dp_value = std::get_if<Bitmask32DatapointValue>(&datapoint.value)) {
      ESP_LOGV(TAG, "MCU reported sensor %u is: %" PRIx32, datapoint.number, dp_value->value);
      this->publish_state(dp_value->value);
    }
  });
}

void UyatSensor::dump_config() {
  LOG_SENSOR("", "Uyat Sensor", this);
  ESP_LOGCONFIG(TAG, "  Sensor has datapoint ID %u", this->sensor_id_);
}

}  // namespace uyat
}  // namespace esphome
