#include "esphome/core/log.h"
#include "uyat_binary_sensor.h"

namespace esphome {
namespace uyat {

static const char *const TAG = "uyat.binary_sensor";

void UyatBinarySensor::configure_bool_dp(const uint8_t dp_id)
{
  this->dp_binary_sensor_.emplace(std::move(DpBinarySensor::create_for_bool([this](const bool value){on_value(value);}, dp_id)));
}

void UyatBinarySensor::configure_uint_dp(const uint8_t dp_id)
{
  this->dp_binary_sensor_.emplace(std::move(DpBinarySensor::create_for_uint([this](const bool value){on_value(value);}, dp_id)));
}

void UyatBinarySensor::configure_enum_dp(const uint8_t dp_id)
{
  this->dp_binary_sensor_.emplace(std::move(DpBinarySensor::create_for_enum([this](const bool value){on_value(value);}, dp_id)));
}

void UyatBinarySensor::configure_bitmask_dp(const uint8_t dp_id, const uint8_t bit_number)
{
  this->dp_binary_sensor_.emplace(std::move(DpBinarySensor::create_for_bitmap([this](const bool value){on_value(value);}, dp_id, bit_number)));
}

void UyatBinarySensor::setup() {
  this->dp_binary_sensor_->init([this](const MatchingDatapoint& matching_dp, const OnDatapointCallback& callback)
  {
    this->parent_->register_datapoint_listener(matching_dp, callback);
  });
}

void UyatBinarySensor::dump_config() {
  ESP_LOGCONFIG(TAG, "Uyat Binary Sensor:");
  ESP_LOGCONFIG(TAG, "  Binary Sensor is %s", this->dp_binary_sensor_? this->dp_binary_sensor_->to_string().c_str() : "misconfigured!");
}

void UyatBinarySensor::on_value(const bool value)
{
  this->publish_state(value);
}

}  // namespace uyat
}  // namespace esphome
