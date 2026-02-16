#include "esphome/core/log.h"
#include "esphome/core/entity_base.h"

#include "uyat_text_sensor_mapped.h"

namespace esphome {
namespace uyat {

UyatTextSensorMapped::UyatTextSensorMapped(Uyat *parent, Config config):
parent_(*parent),
dp_number_([this](const float value){
            this->on_value(value);
          },
          std::move(config.matching_dp),
          0, 1.0f),
mapping_(std::move(config.mapping))
{}

void UyatTextSensorMapped::setup() {
  this->dp_number_.init(this->parent_);
}

void UyatTextSensorMapped::dump_config() {
  ESP_LOGCONFIG(UyatTextSensorMapped::TAG, "Uyat Mapped Text Sensor:");
  ESP_LOGCONFIG(UyatTextSensorMapped::TAG, "  Text Sensor %s is %s", get_object_id().c_str(), this->dp_number_.get_config().to_string().c_str());
  ESP_LOGCONFIG(UyatTextSensorMapped::TAG, "  Options are:");
  for (const auto& item: this->mapping_) {
    ESP_LOGCONFIG(UyatTextSensorMapped::TAG, "    %u: %s", item.first, item.second.c_str());
  }
}

void UyatTextSensorMapped::on_value(const float value)
{
  ESP_LOGV(UyatTextSensorMapped::TAG, "MCU reported %s is: %.0f", get_object_id().c_str(), value);
  const auto translated = translate(static_cast<uint32_t>(value));
  if (!translated.empty())
  {
    this->publish_state(translated);
  }
  else
  {
    ESP_LOGW(UyatTextSensorMapped::TAG, "Received unmapped value %.0f for %s", value, get_object_id().c_str());
  }
}

std::string UyatTextSensorMapped::translate(const uint32_t number_value) const
{
  auto it = std::find_if(this->mapping_.cbegin(), this->mapping_.cend(), [number_value](const auto& v){ return v.first == number_value; });
  if (it == this->mapping_.cend()) {
    return {};
  }
  return it->second;
}

}  // namespace uyat
}  // namespace esphome
