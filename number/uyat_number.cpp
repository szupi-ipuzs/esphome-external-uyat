#include "esphome/core/log.h"
#include "esphome/core/entity_base.h"

#include "uyat_number.h"

namespace esphome {
namespace uyat {

static const char *const TAG = "uyat.number";

void UyatNumber::configure_bool_dp(const uint8_t dp_id, const uint8_t scale)
{
  this->dp_number_.emplace(std::move(DpNumber::create_for_bool([this](const float value){on_value(value);}, dp_id, 0, scale)));
}

void UyatNumber::configure_uint_dp(const uint8_t dp_id, const uint8_t scale)
{
  this->dp_number_.emplace(std::move(DpNumber::create_for_uint([this](const float value){on_value(value);}, dp_id, 0, scale)));
}

void UyatNumber::configure_enum_dp(const uint8_t dp_id, const uint8_t scale)
{
  this->dp_number_.emplace(std::move(DpNumber::create_for_enum([this](const float value){on_value(value);}, dp_id, 0, scale)));
}

void UyatNumber::setup() {
  assert(this->parent_);
  this->dp_number_->init(*(this->parent_));
}

void UyatNumber::control(float value) {
  ESP_LOGV(TAG, "Setting number %s to %f", get_object_id().c_str(), value);
  this->dp_number_->set_value(value);
  this->publish_state(value);
}

void UyatNumber::dump_config() {
  LOG_NUMBER("", "Uyat Number", this);
  ESP_LOGCONFIG(TAG, "  Number %s is %s", get_object_id().c_str(), this->dp_number_? this->dp_number_->config_to_string().c_str() : "misconfigured!");
}

void UyatNumber::on_value(const float value)
{
  ESP_LOGV(TAG, "MCU reported %s is: %.4f", get_object_id().c_str(), value);
  this->publish_state(value);
}

std::string UyatNumber::get_object_id() const
{
  char object_id_buf[OBJECT_ID_MAX_LEN];
  return this->get_object_id_to(object_id_buf).str();
}

}  // namespace uyat
}  // namespace esphome
