#include "esphome/core/log.h"
#include "esphome/core/entity_base.h"

#include "uyat_number.h"

namespace esphome {
namespace uyat {

static const char *const TAG = "uyat.number";

void UyatNumber::setup() {
  if (!this->parent_)
  {
     ESP_LOGE(TAG, "Uyat parent not set for %s", this->get_object_id().c_str());
     return;
  }

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
