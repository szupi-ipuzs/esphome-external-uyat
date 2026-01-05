#include "esphome/core/log.h"
#include "esphome/core/entity_base.h"
#include "uyat_sensor_vap.h"

namespace esphome::uyat
{

static const char *const TAG = "uyat.sensorVAP";

void UyatSensorVAP::setup() {
  assert(this->parent_);
  this->dp_vap_->init(*(this->parent_));
}

void UyatSensorVAP::dump_config() {
  LOG_SENSOR("", "Uyat VAP Sensor", this);
  ESP_LOGCONFIG(TAG, "  VAP Sensor %s is %s", get_object_id().c_str(), this->dp_vap_? this->dp_vap_->config_to_string().c_str() : "misconfigured!");
}

void UyatSensorVAP::on_value(const DpVAP::VAPValue& value)
{
  ESP_LOGV(TAG, "MCU reported %s is: %.4f", get_object_id().c_str(), value.to_string().c_str());
  if (this->value_type_ == UyatVAPValueType::VOLTAGE)
    this->publish_state(static_cast<float>(value.v));
  else if (this->value_type_ == UyatVAPValueType::AMPERAGE)
    this->publish_state(static_cast<float>(value.a));
  else if (this->value_type_ == UyatVAPValueType::POWER)
    this->publish_state(static_cast<float>(value.p));
}

std::string UyatSensorVAP::get_object_id() const
{
  char object_id_buf[OBJECT_ID_MAX_LEN];
  return this->get_object_id_to(object_id_buf).str();
}

}  // namespace
