#include "uyat_text_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace uyat {

static const char *const TAG = "uyat.text_sensor";

void UyatTextSensor::setup() {
  this->parent_->register_datapoint_listener(this->sensor_id_, [this](const UyatDatapoint &datapoint) {

    if (auto * dp_value = std::get_if<StringDatapointValue>(&datapoint.value))
    {
      ESP_LOGD(TAG, "MCU reported text sensor %u is: %s", datapoint.number, dp_value->value.c_str());
      this->publish_state(dp_value->value);
    }
    else
    if (auto * dp_value = std::get_if<RawDatapointValue>(&datapoint.value))
    {
      std::string data = format_hex_pretty(dp_value->value);
      ESP_LOGD(TAG, "MCU reported text sensor %u is: %s", datapoint.number, data.c_str());
      this->publish_state(data);
    }
    else
    if (auto * dp_value = std::get_if<EnumDatapointValue>(&datapoint.value))
    {
      std::string data = to_string(dp_value->value);
      ESP_LOGD(TAG, "MCU reported text sensor %u is: %s", datapoint.number, data.c_str());
      this->publish_state(data);
    }
    else
    {
      ESP_LOGW(TAG, "Unexpected datapoint type!");
      return;
    }
  });
}

void UyatTextSensor::dump_config() {
  ESP_LOGCONFIG(TAG, "Uyat Text Sensor:");
  ESP_LOGCONFIG(TAG, "  Text Sensor has datapoint ID %u", this->sensor_id_);
}

}  // namespace uyat
}  // namespace esphome
