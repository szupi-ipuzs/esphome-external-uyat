#include "esphome/core/log.h"
#include "esphome/core/entity_base.h"
#include "uyat_select.h"

namespace esphome {
namespace uyat {

UyatSelect::UyatSelect(Uyat *parent, Config config):
parent_(*parent),
optimistic_(config.optimistic),
mappings_(std::move(config.mappings)),
dp_number_([this](const float value){this->on_value(value);},
           std::move(config.matching_dp),
           0, 1.0f)
{}

void UyatSelect::setup() {
  this->dp_number_.init(this->parent_);
}

void UyatSelect::control(size_t index) {
  if (this->optimistic_)
    this->publish_state(index);

  ESP_LOGV(UyatSelect::TAG, "Setting %s to %zu (%s)", get_name().c_str(), index, this->option_at(index));

  if (index >= this->mappings_.size()) {
    ESP_LOGW(UyatSelect::TAG, "Index %zu out of range for %s", index, get_name().c_str());
    return;
  }
  uint32_t mapping = this->mappings_.at(index);
  this->dp_number_.set_value(static_cast<float>(mapping));
}

void UyatSelect::on_value(const float value)
{
  ESP_LOGV(UyatSelect::TAG, "MCU reported %s is: %.0f", get_name().c_str(), value);
  auto translated = translate(static_cast<uint32_t>(value));
  if (translated)
  {
    this->publish_state(translated.value());
  }
  else
  {
    ESP_LOGW(UyatSelect::TAG, "Received unmapped value %.0f for %s", value, get_name().c_str());
  }
}

void UyatSelect::dump_config() {
  LOG_SELECT("", "Uyat Select", this);
  ESP_LOGCONFIG(UyatSelect::TAG, "  Select %s is %s", get_name().c_str(), this->dp_number_.get_config().to_string().c_str());
  ESP_LOGCONFIG(UyatSelect::TAG, "  Options are:");
  const auto &options = this->traits.get_options();
  for (size_t i = 0; i < this->mappings_.size(); i++) {
    ESP_LOGCONFIG(UyatSelect::TAG, "    %i: %s", this->mappings_.at(i), options.at(i));
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
