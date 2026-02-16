#include "esphome/core/log.h"
#include "esphome/core/entity_base.h"

#include "uyat_number.h"

namespace esphome {
namespace uyat {

UyatNumber::UyatNumber(Uyat *parent, Config config):
parent_(*parent),
dp_number_([this](const float value){this->on_value(value);},
            std::move(config.matching_dp),
            config.offset, config.multiplier)
{}

void UyatNumber::setup() {
  this->dp_number_.init(this->parent_);
}

void UyatNumber::control(float value) {
  ESP_LOGV(UyatNumber::TAG, "Setting number %s to %f", get_object_id().c_str(), value);
  this->dp_number_.set_value(value);
  this->publish_state(value);
}

void UyatNumber::dump_config() {
  LOG_NUMBER("", "Uyat Number", this);
  ESP_LOGCONFIG(UyatNumber::TAG, "  Number %s is %s", get_object_id().c_str(), this->dp_number_.get_config().to_string().c_str());
}

void UyatNumber::on_value(const float value)
{
  ESP_LOGV(UyatNumber::TAG, "MCU reported %s is: %.4f", get_object_id().c_str(), value);
  this->publish_state(value);
}

}  // namespace uyat
}  // namespace esphome
