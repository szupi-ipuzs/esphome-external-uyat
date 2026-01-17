#include "esphome/core/log.h"
#include "uyat_cover.h"

namespace esphome {
namespace uyat {

const uint8_t COMMAND_OPEN = 0x00;
const uint8_t COMMAND_CLOSE = 0x02;
const uint8_t COMMAND_STOP = 0x01;

using namespace esphome::cover;

void UyatCover::on_position_value(const float value_percent)
{
  ESP_LOGD(UyatCover::TAG, "MCU reported position %.0f%%", value_percent * 100);
  this->position = value_percent;
  this->publish_state();
}

void UyatCover::setup() {

  this->position_.init(*(this->parent_));
  if (this->control_.has_value())
  {
    this->control_->init(*(this->parent_));
  }
  if (this->direction_.has_value())
  {
    this->direction_->init(*(this->parent_));
  }

  this->parent_->add_on_initialized_callback([this]() {
    // Set the direction (if configured/supported).
    this->apply_direction_();

    // Handle configured restore mode.
    switch (this->restore_mode_) {
      case COVER_NO_RESTORE:
        break;
      case COVER_RESTORE: {
        auto restore = this->restore_state_();
        if (restore.has_value())
          restore->apply(this);
        break;
      }
      case COVER_RESTORE_AND_CALL: {
        auto restore = this->restore_state_();
        if (restore.has_value()) {
          restore->to_call(this).perform();
        }
        break;
      }
    }
  });
}

void UyatCover::control(const cover::CoverCall &call) {
  if (call.get_stop()) {
    if (this->control_.has_value() && this->control_->supports_stop()) {
      ESP_LOGD(UyatCover::TAG, "Sending stop via command");
      this->control_->stop();
    } else if (this->position_.supports_setting_position()){
      ESP_LOGD(UyatCover::TAG, "Sending stop via position (%0.0f)", this->position * 100);
      this->position_.set_position(this->position);
    }
    else
    {
      ESP_LOGE(UyatCover::TAG, "Misconfiguration! Could not stop");
    }
  }
  if (call.get_position().has_value()) {
    auto pos = *call.get_position();
    bool handled_by_command = false;
    if (pos == COVER_OPEN)
    {
      if (this->control_.has_value() && (this->control_->supports_open()))
      {
        ESP_LOGD(UyatCover::TAG, "Sending open command");
        this->control_->open();
        handled_by_command = true;
      }
    }
    else
    if (pos == COVER_CLOSED)
    {
      if (this->control_.has_value() && (this->control_->supports_close()))
      {
        ESP_LOGD(UyatCover::TAG, "Sending close command");
        this->control_->close();
        handled_by_command = true;
      }
    }
    else {}

    if (!handled_by_command)
    {
      if (this->position_.supports_setting_position()){
        ESP_LOGD(UyatCover::TAG, "Setting position to %.0f%%", pos * 100);
        this->position_.set_position(pos);
      }
      else
      {
        ESP_LOGE(UyatCover::TAG, "Misconfiguration! Could not set position!");
      }
    }
  }

  this->publish_state();
}

void UyatCover::apply_direction_() {
  if (this->direction_.has_value()) {
    this->direction_->set_value(true);
  }
}

void UyatCover::dump_config() {
  ESP_LOGCONFIG(UyatCover::TAG, "Uyat Cover:");
  if (this->control_.has_value()) {
    this->control_->dump_config();
  }
  if (this->direction_.has_value()) {
    ESP_LOGCONFIG(UyatCover::TAG, "   Direction is %s", this->direction_->get_config().to_string().c_str());
  }
  this->position_.dump_config();
}

cover::CoverTraits UyatCover::get_traits() {
  auto traits = cover::CoverTraits();
  traits.set_supports_stop((this->control_.has_value() && this->control_->supports_stop()) || (this->position_.supports_setting_position()));
  traits.set_supports_position(this->position_.supports_setting_position());
  return traits;
}

}  // namespace uyat
}  // namespace esphome
