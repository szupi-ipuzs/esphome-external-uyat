#include "esphome/core/log.h"

#include "automation.h"

static const char *const TAG = "uyat.automation";

namespace esphome {
namespace uyat {

UyatRawDatapointUpdateTrigger::UyatRawDatapointUpdateTrigger(Uyat *parent, uint8_t sensor_id) {
  parent->register_datapoint_listener(MatchingDatapoint{.number = sensor_id, .types = {UyatDatapointType::RAW}}, [this](const UyatDatapoint &dp) {
    auto * dp_value = std::get_if<RawDatapointValue>(&dp.value);
    if (!dp_value)
    {
      ESP_LOGW(TAG, "Unexpected datapoint %d type (expected RAW, got %s)!", dp.number, dp.get_type_name());
      return;
    }
    this->trigger(dp_value->value);
  });
}

UyatBoolDatapointUpdateTrigger::UyatBoolDatapointUpdateTrigger(Uyat *parent, uint8_t sensor_id) {
  parent->register_datapoint_listener(MatchingDatapoint{.number = sensor_id, .types = {UyatDatapointType::BOOLEAN}}, [this](const UyatDatapoint &dp) {
    auto * dp_value = std::get_if<BoolDatapointValue>(&dp.value);
    if (!dp_value)
    {
      ESP_LOGW(TAG, "Unexpected datapoint %d type (expected BOOL, got %s)!", dp.number, dp.get_type_name());
      return;
    }
    this->trigger(dp_value->value);
  });
}

UyatUIntDatapointUpdateTrigger::UyatUIntDatapointUpdateTrigger(Uyat *parent, uint8_t sensor_id) {
  parent->register_datapoint_listener(MatchingDatapoint{.number = sensor_id, .types = {UyatDatapointType::INTEGER}}, [this](const UyatDatapoint &dp) {
    auto * dp_value = std::get_if<UIntDatapointValue>(&dp.value);
    if (!dp_value)
    {
      ESP_LOGW(TAG, "Unexpected datapoint %d type (expected INTEGER, got %s)!", dp.number, dp.get_type_name());
      return;
    }
    this->trigger(dp_value->value);
  });
}

UyatStringDatapointUpdateTrigger::UyatStringDatapointUpdateTrigger(Uyat *parent, uint8_t sensor_id) {
  parent->register_datapoint_listener(MatchingDatapoint{.number = sensor_id, .types = {UyatDatapointType::STRING}}, [this](const UyatDatapoint &dp) {
    auto * dp_value = std::get_if<StringDatapointValue>(&dp.value);
    if (!dp_value)
    {
      ESP_LOGW(TAG, "Unexpected datapoint %d type (expected STRING, got %s)!", dp.number, dp.get_type_name());
      return;
    }
    this->trigger(dp_value->value);
  });
}

UyatEnumDatapointUpdateTrigger::UyatEnumDatapointUpdateTrigger(Uyat *parent, uint8_t sensor_id) {
  parent->register_datapoint_listener(MatchingDatapoint{.number = sensor_id, .types = {UyatDatapointType::ENUM}}, [this](const UyatDatapoint &dp) {
    auto * dp_value = std::get_if<EnumDatapointValue>(&dp.value);
    if (!dp_value)
    {
      ESP_LOGW(TAG, "Unexpected datapoint %d type (expected ENUM, got %s)!", dp.number, dp.get_type_name());
      return;
    }
    this->trigger(dp_value->value);
  });
}

UyatBitmapDatapointUpdateTrigger::UyatBitmapDatapointUpdateTrigger(Uyat *parent, uint8_t sensor_id) {
  parent->register_datapoint_listener(MatchingDatapoint{.number = sensor_id, .types = {UyatDatapointType::BITMAP}}, [this](const UyatDatapoint &dp) {
    auto * dp_value = std::get_if<BitmapDatapointValue>(&dp.value);
    if (!dp_value)
    {
      ESP_LOGW(TAG, "Unexpected datapoint %d type (expected BITMAP, got %s)!", dp.number, dp.get_type_name());
      return;
    }
    this->trigger(dp_value->value);
  });
}

}  // namespace uyat
}  // namespace esphome
