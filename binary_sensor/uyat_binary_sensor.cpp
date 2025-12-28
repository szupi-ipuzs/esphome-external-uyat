#include "esphome/core/log.h"
#include "uyat_binary_sensor.h"

namespace esphome {
namespace uyat {

static const char *const TAG = "uyat.binary_sensor";

void UyatBinarySensor::setup() {
  this->parent_->register_listener(this->matching_dp_, [this](const UyatDatapoint &datapoint) {
    if (!datapoint.matches(matching_dp_))
    {
      ESP_LOGW(TAG, "Unexpected datapoint type!");
      return;
    }

    ESP_LOGV(TAG, "MCU reported binary sensor %s", datapoint.to_string());

    if (auto * dp_value = std::get_if<BoolDatapointValue>(&datapoint.value))
    {
      this->publish_state(dp_value->value);
    }
    else
    if (auto * dp_value = std::get_if<UIntDatapointValue>(&datapoint.value))
    {
      this->publish_state(dp_value->value != 0);
    }
    else
    if (auto * dp_value = std::get_if<EnumDatapointValue>(&datapoint.value))
    {
      this->publish_state(dp_value->value != 0);
    }
    else
    {
      ESP_LOGW(TAG, "Unhandled datapoint type %s!", datapoint.get_type_name());
      return;
    }
  });
}

void UyatBinarySensor::dump_config() {
  ESP_LOGCONFIG(TAG, "Uyat Binary Sensor:");
  ESP_LOGCONFIG(TAG, "  Binary Sensor is %s", this->matching_dp_.to_string());
}

}  // namespace uyat
}  // namespace esphome
