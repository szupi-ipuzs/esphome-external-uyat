#include "esphome/core/log.h"
#include "esphome/core/entity_base.h"
#include "uyat_sensor_vap.h"

namespace esphome::uyat
{

UyatSensorVAP::UyatSensorVAP(Uyat *parent, Config config):
parent_(*parent),
dp_vap_([this](const DpVAP::VAPValue& value){this->on_value(value);},
          std::move(config.matching_dp)),
value_type_(config.value_type)
{}

void UyatSensorVAP::setup() {
  this->dp_vap_.init(this->parent_);
}

void UyatSensorVAP::dump_config() {
  LOG_SENSOR("", "Uyat VAP Sensor", this);
  char config_str[UYAT_LOG_BUFFER_SIZE];
  this->dp_vap_.get_config().to_string(config_str, sizeof(config_str));
  ESP_LOGCONFIG(UyatSensorVAP::TAG, "  VAP Sensor %s is %s", get_name().c_str(), config_str);
}

void UyatSensorVAP::on_value(const DpVAP::VAPValue& value)
{
  char value_str[UYAT_LOG_BUFFER_SIZE];
  value.to_string(value_str, sizeof(value_str));
  ESP_LOGV(UyatSensorVAP::TAG, "MCU reported %s is: %s", get_name().c_str(), value_str);
  if (this->value_type_ == UyatVAPValueType::VOLTAGE)
    this->publish_state(static_cast<float>(value.v));
  else if (this->value_type_ == UyatVAPValueType::AMPERAGE)
    this->publish_state(static_cast<float>(value.a));
  else if (this->value_type_ == UyatVAPValueType::POWER)
    this->publish_state(static_cast<float>(value.p));
}


}  // namespace
