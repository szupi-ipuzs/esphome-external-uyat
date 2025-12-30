#include "esphome/core/log.h"
#include "uyat_number.h"

namespace esphome {
namespace uyat {

static const char *const TAG = "uyat.number";

void UyatNumber::setup() {
  if (this->restore_value_) {
    this->pref_ = global_preferences->make_preference<float>(this->get_preference_hash());
  }

  this->parent_->register_datapoint_listener(this->number_id_, [this](const UyatDatapoint &datapoint) {
    if (auto * dp_value = std::get_if<UIntDatapointValue>(&datapoint.value))
    {
      ESP_LOGV(TAG, "MCU reported number %u is: %s", datapoint.number, dp_value->to_string());
      float value = dp_value->value / this->multiply_by_;
      this->publish_state(value);
      if (this->restore_value_)
        this->pref_.save(&value);
    }
    else
    if (auto * dp_value = std::get_if<EnumDatapointValue>(&datapoint.value))
    {
      ESP_LOGV(TAG, "MCU reported number %u is: %s", datapoint.number, dp_value->to_string());
      float value = dp_value->value;
      this->publish_state(value);
      if (this->restore_value_)
        this->pref_.save(&value);
    }
    else
    {
      ESP_LOGW(TAG, "Unexpected datapoint %d type (expected INTEGER or ENUM, got %s)!", datapoint.number, datapoint.get_type_name());
      return;
    }

    if ((this->type_) && (this->type_ != datapoint.get_type())) {
      ESP_LOGW(TAG, "Reported type (%d) different than previously set (%d)!", static_cast<int>(datapoint.get_type()),
               static_cast<int>(*this->type_));
    }
    this->type_ = datapoint.get_type();
  });

  this->parent_->add_on_initialized_callback([this] {
    if (this->type_) {
      float value;
      if (!this->restore_value_) {
        if (this->initial_value_) {
          value = *this->initial_value_;
        } else {
          return;
        }
      } else {
        if (!this->pref_.load(&value)) {
          if (this->initial_value_) {
            value = *this->initial_value_;
          } else {
            value = this->traits.get_min_value();
            ESP_LOGW(TAG, "Failed to restore and there is no initial value defined. Setting min_value (%f)", value);
          }
        }
      }

      this->control(value);
    }
  });
}

void UyatNumber::control(float value) {
  ESP_LOGV(TAG, "Setting number %u: %f", this->number_id_, value);
  if (this->type_ == UyatDatapointType::INTEGER) {
    int integer_value = lround(value * multiply_by_);
    this->parent_->set_integer_datapoint_value(this->number_id_, integer_value);
  } else if (this->type_ == UyatDatapointType::ENUM) {
    this->parent_->set_enum_datapoint_value(this->number_id_, value);
  }
  this->publish_state(value);

  if (this->restore_value_)
    this->pref_.save(&value);
}

void UyatNumber::dump_config() {
  LOG_NUMBER("", "Uyat Number", this);
  ESP_LOGCONFIG(TAG, "  Number has datapoint ID %u", this->number_id_);
  if (this->type_) {
    ESP_LOGCONFIG(TAG, "  Datapoint type is %d", static_cast<int>(*this->type_));
  } else {
    ESP_LOGCONFIG(TAG, "  Datapoint type is unknown");
  }

  if (this->initial_value_) {
    ESP_LOGCONFIG(TAG, "  Initial Value: %f", *this->initial_value_);
  }

  ESP_LOGCONFIG(TAG, "  Restore Value: %s", YESNO(this->restore_value_));
}

}  // namespace uyat
}  // namespace esphome
