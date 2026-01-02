#include "esphome/core/log.h"
#include "esphome/core/entity_base.h"

#include "uyat_text_sensor_mapped.h"

namespace esphome {
namespace uyat {

static const char *const TAG = "uyat.text_sensor_mapped";

void UyatTextSensorMapped::configure_bool_dp(const uint8_t dp_id)
{
  this->dp_number_.emplace(std::move(DpNumber::create_for_bool([this](const float value){on_value(value);}, dp_id)));
}

void UyatTextSensorMapped::configure_uint_dp(const uint8_t dp_id)
{
  this->dp_number_.emplace(std::move(DpNumber::create_for_uint([this](const float value){on_value(value);}, dp_id)));
}

void UyatTextSensorMapped::configure_enum_dp(const uint8_t dp_id)
{
  this->dp_number_.emplace(std::move(DpNumber::create_for_enum([this](const float value){on_value(value);}, dp_id)));
}

void UyatTextSensorMapped::setup() {
  assert(this->parent_);
  this->dp_number_->init(*(this->parent_));
}

void UyatTextSensorMapped::dump_config() {
  ESP_LOGCONFIG(TAG, "Uyat Mapped Text Sensor:");
  ESP_LOGCONFIG(TAG, "  Text Sensor %s is %s", get_object_id().c_str(), this->dp_number_? this->dp_number_->config_to_string().c_str() : "misconfigured!");
  ESP_LOGCONFIG(TAG, "  Options are:");
  for (const auto& item: this->mappings_) {
    ESP_LOGCONFIG(TAG, "    %u: %s", item.first, item.second.c_str());
  }
}

void UyatTextSensorMapped::on_value(const float value)
{
  ESP_LOGV(TAG, "MCU reported %s is: %.0f", get_object_id().c_str(), value);
  const auto translated = translate(static_cast<uint32_t>(value));
  if (!translated.empty())
  {
    this->publish_state(translated);
  }
  else
  {
    ESP_LOGW(TAG, "Received unmapped value %.0f for %s", value, get_object_id().c_str());
  }
}

std::string UyatTextSensorMapped::get_object_id() const
{
  char object_id_buf[OBJECT_ID_MAX_LEN];
  return this->get_object_id_to(object_id_buf).str();
}

std::string UyatTextSensorMapped::translate(const uint32_t number_value) const
{
  auto it = std::find_if(this->mappings_.cbegin(), this->mappings_.cend(), [number_value](const auto& v){ return v.first == number_value; });
  if (it == this->mappings_.cend()) {
    return {};
  }
  return it->second;
}

}  // namespace uyat
}  // namespace esphome
