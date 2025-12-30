#include "esphome/core/log.h"

#include "automation.h"

static const char *const TAG = "uyat.automation";

namespace esphome {
namespace uyat {

void check_expected_datapoint(const UyatDatapoint &dp, UyatDatapointType expected) {
  if (dp.get_type() != expected) {
    ESP_LOGW(TAG, "Uyat sensor %u expected datapoint type %#02hhX but got %#02hhX", dp.number, dp.number,
             static_cast<uint8_t>(expected), static_cast<uint8_t>(dp.get_type()));
  }
}

UyatRawDatapointUpdateTrigger::UyatRawDatapointUpdateTrigger(Uyat *parent, uint8_t sensor_id) {
  parent->register_datapoint_listener(sensor_id, [this](const UyatDatapoint &dp) {
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
  parent->register_datapoint_listener(sensor_id, [this](const UyatDatapoint &dp) {
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
  parent->register_datapoint_listener(sensor_id, [this](const UyatDatapoint &dp) {
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
  parent->register_datapoint_listener(sensor_id, [this](const UyatDatapoint &dp) {
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
  parent->register_datapoint_listener(sensor_id, [this](const UyatDatapoint &dp) {
    auto * dp_value = std::get_if<EnumDatapointValue>(&dp.value);
    if (!dp_value)
    {
      ESP_LOGW(TAG, "Unexpected datapoint %d type (expected ENUM, got %s)!", dp.number, dp.get_type_name());
      return;
    }
    this->trigger(dp_value->value);
  });
}

UyatBitmask8DatapointUpdateTrigger::UyatBitmask8DatapointUpdateTrigger(Uyat *parent, uint8_t sensor_id) {
  parent->register_datapoint_listener(sensor_id, [this](const UyatDatapoint &dp) {
    auto * dp_value = std::get_if<Bitmask8DatapointValue>(&dp.value);
    if (!dp_value)
    {
      ESP_LOGW(TAG, "Unexpected datapoint %d type (expected BITMASK8, got %s)!", dp.number, dp.get_type_name());
      return;
    }
    this->trigger(dp_value->value);
  });
}

UyatBitmask16DatapointUpdateTrigger::UyatBitmask16DatapointUpdateTrigger(Uyat *parent, uint8_t sensor_id) {
  parent->register_datapoint_listener(sensor_id, [this](const UyatDatapoint &dp) {
    auto * dp_value = std::get_if<Bitmask16DatapointValue>(&dp.value);
    if (!dp_value)
    {
      ESP_LOGW(TAG, "Unexpected datapoint %d type (expected BITMASK16, got %s)!", dp.number, dp.get_type_name());
      return;
    }
    this->trigger(dp_value->value);
  });
}

UyatBitmask32DatapointUpdateTrigger::UyatBitmask32DatapointUpdateTrigger(Uyat *parent, uint8_t sensor_id) {
  parent->register_datapoint_listener(sensor_id, [this](const UyatDatapoint &dp) {
    auto * dp_value = std::get_if<Bitmask32DatapointValue>(&dp.value);
    if (!dp_value)
    {
      ESP_LOGW(TAG, "Unexpected datapoint %d type (expected BITMASK32, got %s)!", dp.number, dp.get_type_name());
      return;
    }
    this->trigger(dp_value->value);
  });
}

}  // namespace uyat
}  // namespace esphome
