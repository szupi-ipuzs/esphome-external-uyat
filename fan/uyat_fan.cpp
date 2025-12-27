#include "esphome/core/log.h"
#include "uyat_fan.h"

namespace esphome {
namespace uyat {

static const char *const TAG = "uyat.fan";

void UyatFan::setup() {
  if (this->speed_id_.has_value()) {
    this->parent_->register_listener(*this->speed_id_, [this](const UyatDatapoint &datapoint) {
      if (auto * dp_value = std::get_if<EnumDatapointValue>(&datapoint.value)) {
        ESP_LOGV(TAG, "MCU reported speed of: %d", dp_value->value);
        if (dp_value->value >= this->speed_count_) {
          ESP_LOGE(TAG, "Speed has invalid value %d", dp_value->value);
        } else {
          this->speed = dp_value->value + 1;
          this->publish_state();
        }
      }
      else
      if (auto * dp_value = std::get_if<UIntDatapointValue>(&datapoint.value)) {
        ESP_LOGV(TAG, "MCU reported speed of: %d", dp_value->value);
        this->speed = dp_value->value;
        this->publish_state();
      }
      else
      {
        ESP_LOGW(TAG, "Unexpected datapoint %d type (expected INTEGER or ENUM, got %s)!", datapoint.number, datapoint.get_type_name());
        return;
      }
      this->speed_type_ = datapoint.get_type();
    });
  }
  if (this->switch_id_.has_value()) {
    this->parent_->register_listener(*this->switch_id_, [this](const UyatDatapoint &datapoint) {
      auto * dp_value = std::get_if<BoolDatapointValue>(&datapoint.value);
      if (!dp_value)
      {
        ESP_LOGW(TAG, "Unexpected datapoint type!");
        return;
      }

      ESP_LOGV(TAG, "MCU reported switch %u is: %s", this->switch_id_, ONOFF(dp_value->value));
      this->state = dp_value->value;
      this->publish_state();
    });
  }
  if (this->oscillation_id_.has_value()) {
    this->parent_->register_listener(*this->oscillation_id_, [this](const UyatDatapoint &datapoint) {
      // Whether data type is BOOL or ENUM, it will still be a 1 or a 0, so the functions below are valid in both
      // scenarios
      if (auto * dp_value = std::get_if<BoolDatapointValue>(&datapoint.value))
      {
        this->oscillating = dp_value->value;
      }
      else
      if (auto * dp_value = std::get_if<EnumDatapointValue>(&datapoint.value))
      {
        this->oscillating = (dp_value->value != 0);
      }
      else
      {
        ESP_LOGW(TAG, "Unexpected datapoint type!");
        return;
      }

      ESP_LOGV(TAG, "MCU reported oscillation is: %s", ONOFF(this->oscillating));
      this->publish_state();

      this->oscillation_type_ = datapoint.get_type();
    });
  }
  if (this->direction_id_.has_value()) {
    this->parent_->register_listener(*this->direction_id_, [this](const UyatDatapoint &datapoint) {
      auto * dp_value = std::get_if<BoolDatapointValue>(&datapoint.value);
      if (!dp_value)
      {
        ESP_LOGW(TAG, "Unexpected datapoint type!");
        return;
      }

      ESP_LOGD(TAG, "MCU reported reverse direction is: %s", ONOFF(dp_value->value));
      this->direction = dp_value->value ? fan::FanDirection::REVERSE : fan::FanDirection::FORWARD;
      this->publish_state();
    });
  }

  this->parent_->add_on_initialized_callback([this]() {
    auto restored = this->restore_state_();
    if (restored)
      restored->to_call(*this).perform();
  });
}

void UyatFan::dump_config() {
  LOG_FAN("", "Uyat Fan", this);
  if (this->speed_id_.has_value()) {
    ESP_LOGCONFIG(TAG, "  Speed has datapoint ID %u", *this->speed_id_);
  }
  if (this->switch_id_.has_value()) {
    ESP_LOGCONFIG(TAG, "  Switch has datapoint ID %u", *this->switch_id_);
  }
  if (this->oscillation_id_.has_value()) {
    ESP_LOGCONFIG(TAG, "  Oscillation has datapoint ID %u", *this->oscillation_id_);
  }
  if (this->direction_id_.has_value()) {
    ESP_LOGCONFIG(TAG, "  Direction has datapoint ID %u", *this->direction_id_);
  }
}

fan::FanTraits UyatFan::get_traits() {
  return fan::FanTraits(this->oscillation_id_.has_value(), this->speed_id_.has_value(), this->direction_id_.has_value(),
                        this->speed_count_);
}

void UyatFan::control(const fan::FanCall &call) {
  if (this->switch_id_.has_value() && call.get_state().has_value()) {
    this->parent_->set_boolean_datapoint_value(*this->switch_id_, *call.get_state());
  }
  if (this->oscillation_id_.has_value() && call.get_oscillating().has_value()) {
    if (this->oscillation_type_ == UyatDatapointType::ENUM) {
      this->parent_->set_enum_datapoint_value(*this->oscillation_id_, *call.get_oscillating());
    } else if (this->oscillation_type_ == UyatDatapointType::BOOLEAN) {
      this->parent_->set_boolean_datapoint_value(*this->oscillation_id_, *call.get_oscillating());
    }
  }
  if (this->direction_id_.has_value() && call.get_direction().has_value()) {
    bool enable = *call.get_direction() == fan::FanDirection::REVERSE;
    this->parent_->set_enum_datapoint_value(*this->direction_id_, enable);
  }
  if (this->speed_id_.has_value() && call.get_speed().has_value()) {
    if (this->speed_type_ == UyatDatapointType::ENUM) {
      this->parent_->set_enum_datapoint_value(*this->speed_id_, *call.get_speed() - 1);
    } else if (this->speed_type_ == UyatDatapointType::INTEGER) {
      this->parent_->set_integer_datapoint_value(*this->speed_id_, *call.get_speed());
    }
  }
}

}  // namespace uyat
}  // namespace esphome
