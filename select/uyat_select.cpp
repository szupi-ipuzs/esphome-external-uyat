#include "esphome/core/log.h"
#include "uyat_select.h"

namespace esphome {
namespace uyat {

static const char *const TAG = "uyat.select";

void UyatSelect::setup() {
  this->parent_->register_datapoint_listener(this->select_id_, [this](const UyatDatapoint &datapoint) {
    uint8_t enum_value;
    if (this->is_int_)
    {
      if (auto * dp_value = std::get_if<UIntDatapointValue>(&datapoint.value))
      {
        enum_value = dp_value->value;
      }
      else
      {
        ESP_LOGW(TAG, "Unexpected datapoint %d type (expected INTEGER, got %s)!", datapoint.number, datapoint.get_type_name());
        return;
      }
    }
    else
    {
      if (auto * dp_value = std::get_if<EnumDatapointValue>(&datapoint.value))
      {
        enum_value = dp_value->value;
      }
      else
      {
        ESP_LOGW(TAG, "Unexpected datapoint %d type (expected ENUM, got %s)!", datapoint.number, datapoint.get_type_name());
        return;
      }
    }

    ESP_LOGV(TAG, "MCU reported select %u value %u", this->select_id_, enum_value);
    auto mappings = this->mappings_;
    auto it = std::find(mappings.cbegin(), mappings.cend(), enum_value);
    if (it == mappings.end()) {
      ESP_LOGW(TAG, "Invalid value %u", enum_value);
      return;
    }
    size_t mapping_idx = std::distance(mappings.cbegin(), it);
    this->publish_state(mapping_idx);
  });
}

void UyatSelect::control(size_t index) {
  if (this->optimistic_)
    this->publish_state(index);

  uint8_t mapping = this->mappings_.at(index);
  ESP_LOGV(TAG, "Setting %u datapoint value to %u:%s", this->select_id_, mapping, this->option_at(index));
  if (this->is_int_) {
    this->parent_->set_integer_datapoint_value(this->select_id_, mapping);
  } else {
    this->parent_->set_enum_datapoint_value(this->select_id_, mapping);
  }
}

void UyatSelect::dump_config() {
  LOG_SELECT("", "Uyat Select", this);
  ESP_LOGCONFIG(TAG,
                "  Select has datapoint ID %u\n"
                "  Data type: %s\n"
                "  Options are:",
                this->select_id_, this->is_int_ ? "int" : "enum");
  const auto &options = this->traits.get_options();
  for (size_t i = 0; i < this->mappings_.size(); i++) {
    ESP_LOGCONFIG(TAG, "    %i: %s", this->mappings_.at(i), options.at(i));
  }
}

}  // namespace uyat
}  // namespace esphome
