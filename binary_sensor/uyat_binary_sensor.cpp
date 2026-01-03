#include "esphome/core/log.h"
#include "esphome/core/entity_base.h"
#include "uyat_binary_sensor.h"

namespace esphome {
namespace uyat {

static const char *const TAG = "uyat.binary_sensor";

void UyatBinarySensor::configure_any_dp(const uint8_t dp_id)
{
  this->dp_binary_sensor_.emplace(std::move(DpBinarySensor::create_for_any([this](const bool value){on_value(value);}, dp_id)));
}

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

void UyatBinarySensor::configure_bitmap_dp(const uint8_t dp_id, const uint8_t bit_number)
{
  this->dp_binary_sensor_.emplace(std::move(DpBinarySensor::create_for_bitmap([this](const bool value){on_value(value);}, dp_id, bit_number)));
}

void UyatBinarySensor::setup() {
  assert(this->parent_);
  this->dp_binary_sensor_->init(*(this->parent_));
}

void UyatBinarySensor::dump_config() {
  ESP_LOGCONFIG(TAG, "Uyat Binary Sensor:");
  ESP_LOGCONFIG(TAG, "  Binary Sensor %s is %s", get_object_id().c_str(), this->dp_binary_sensor_? this->dp_binary_sensor_->config_to_string().c_str() : "misconfigured!");
}

void UyatBinarySensor::on_value(const bool value)
{
  ESP_LOGV(TAG, "MCU reported %s is: %s", get_object_id().c_str(), ONOFF(value));
  this->publish_state(value);
}

std::string UyatBinarySensor::get_object_id() const
{
  char object_id_buf[OBJECT_ID_MAX_LEN];
  return this->get_object_id_to(object_id_buf).str();
}

}  // namespace uyat
}  // namespace esphome
