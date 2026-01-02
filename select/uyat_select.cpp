#include "esphome/core/log.h"
#include "esphome/core/entity_base.h"
#include "uyat_select.h"

namespace esphome {
namespace uyat {

static const char *const TAG = "uyat.select";

void UyatSelect::configure_bool_dp(const uint8_t dp_id)
{
  this->dp_number_.emplace(std::move(DpNumber::create_for_bool([this](const float value){on_value(value);}, dp_id)));
}

void UyatSelect::configure_uint_dp(const uint8_t dp_id)
{
  this->dp_number_.emplace(std::move(DpNumber::create_for_uint([this](const float value){on_value(value);}, dp_id)));
}

void UyatSelect::configure_enum_dp(const uint8_t dp_id)
{
  this->dp_number_.emplace(std::move(DpNumber::create_for_enum([this](const float value){on_value(value);}, dp_id)));
}

void UyatSelect::setup() {
  assert(this->parent_);
  this->dp_number_->init(*(this->parent_));
}

void UyatSelect::control(size_t index) {
  if (this->optimistic_)
    this->publish_state(index);

  ESP_LOGV(TAG, "Setting %s to %zu (%s)", get_object_id().c_str(), index, this->option_at(index));

  if (index >= this->mappings_.size()) {
    ESP_LOGW(TAG, "Index %zu out of range for %s", index, get_object_id().c_str());
    return;
  }
  uint32_t mapping = this->mappings_.at(index);
  this->dp_number_->set_value(static_cast<float>(mapping));
}

void UyatSelect::on_value(const float value)
{
  ESP_LOGV(TAG, "MCU reported %s is: %.0f", get_object_id().c_str(), value);
  auto translated = translate(static_cast<uint32_t>(value));
  if (translated)
  {
    this->publish_state(translated.value());
  }
  else
  {
    ESP_LOGW(TAG, "Received unmapped value %.0f for %s", value, get_object_id().c_str());
  }
}

std::string UyatSelect::get_object_id() const
{
  char object_id_buf[OBJECT_ID_MAX_LEN];
  return this->get_object_id_to(object_id_buf).str();
}

void UyatSelect::dump_config() {
  LOG_SELECT("", "Uyat Select", this);
  ESP_LOGCONFIG(TAG, "  Select %s is %s", get_object_id().c_str(), this->dp_number_? this->dp_number_->config_to_string().c_str() : "misconfigured!");
  ESP_LOGCONFIG(TAG, "  Options are:");
  const auto &options = this->traits.get_options();
  for (size_t i = 0; i < this->mappings_.size(); i++) {
    ESP_LOGCONFIG(TAG, "    %i: %s", this->mappings_.at(i), options.at(i));
  }
}

std::optional<std::size_t> UyatSelect::translate(const uint32_t number_value) const
{
  auto it = std::find(this->mappings_.cbegin(), this->mappings_.cend(), number_value);
  if (it == this->mappings_.cend()) {
    return std::nullopt;
  }
  return std::distance(this->mappings_.cbegin(), it);
}

}  // namespace uyat
}  // namespace esphome
