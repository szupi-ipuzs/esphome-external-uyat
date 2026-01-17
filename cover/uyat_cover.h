#pragma once

#include "esphome/core/component.h"
#include "esphome/components/cover/cover.h"

#include "../uyat.h"
#include "../dp_number.h"
#include "../dp_dimmer.h"
#include "../dp_switch.h"

namespace esphome {
namespace uyat {

enum UyatCoverRestoreMode {
  COVER_NO_RESTORE,
  COVER_RESTORE,
  COVER_RESTORE_AND_CALL,
};

struct ControlDpValueMapping
{
  std::optional<uint32_t> open_value;
  std::optional<uint32_t> close_value;
  std::optional<uint32_t> stop_value;
};

class UyatCover : public cover::Cover, public Component {
 private:

  static constexpr const char* TAG = "uyat.cover";

  void on_position_value(const float);

 public:
  void setup() override;
  void dump_config() override;
  void configure_control(MatchingDatapoint matching_dp, const ControlDpValueMapping& mapping) {
    this->control_.emplace(
      DpNumber([this](const float){},
               std::move(matching_dp),
               0.0f, 1.0f
      ),
      mapping
    );
  }
  void configure_direction(MatchingDatapoint matching_dp, const bool inverted) {
    this->direction_.emplace([this](const bool){},
                             std::move(matching_dp),
                             inverted
                            );
  }
  void configure_position(std::optional<MatchingDatapoint> position_dp, std::optional<MatchingDatapoint> position_report_dp,
                          const bool inverted,
                          const uint32_t min_value, const uint32_t max_value,
                          std::optional<uint32_t> uncalibrated_value){
    DpDimmer::BrightnessChangedCallback on_position_lambda = [this](const float value){this->on_position_value(value);};
    if (position_report_dp)
    {
      this->position_.report_dp_position.emplace(on_position_lambda,
               std::move(*position_report_dp),
               min_value, max_value, inverted);
      on_position_lambda = [](const float){};
    }

    if (position_dp)
    {
      this->position_.dp_position.emplace(
                  on_position_lambda,
                  std::move(*position_dp),
                  min_value, max_value, inverted);
    }

    this->position_.uncalibrated_value = uncalibrated_value;
  }
  void set_uyat_parent(Uyat *parent) { this->parent_ = parent; }
  void set_restore_mode(UyatCoverRestoreMode restore_mode) { restore_mode_ = restore_mode; }

 protected:

  struct Control
  {
    DpNumber dp_number;
    ControlDpValueMapping mapping;

    void init(DatapointHandler& handler)
    {
      dp_number.init(handler);
    }

    void dump_config() const
    {
      ESP_LOGCONFIG(UyatCover::TAG, "   Control is %s", dp_number.get_config().to_string().c_str());
    }

    bool supports_open() const
    {
      return mapping.open_value.has_value();
    }

    bool supports_close() const
    {
      return mapping.close_value.has_value();
    }

    bool supports_stop() const
    {
      return mapping.stop_value.has_value();
    }

    bool stop()
    {
      if (!mapping.stop_value)
      {
        return false;
      }
      dp_number.set_value(*mapping.stop_value, true);
      return true;
    }
    bool open()
    {
      if (!mapping.open_value)
      {
        return false;
      }
      dp_number.set_value(*mapping.open_value, true);
      return true;
    }
    bool close()
    {
      if (!mapping.close_value)
      {
        return false;
      }
      dp_number.set_value(*mapping.close_value, true);
      return true;
    }
  };

  struct Position
  {
    std::optional<DpDimmer> dp_position;
    std::optional<DpDimmer> report_dp_position;
    std::optional<uint32_t> uncalibrated_value;

    bool supports_setting_position() const
    {
      return dp_position.has_value();
    }

    bool set_position(const float value)
    {
      if (!dp_position.has_value())
      {
        return false;
      }

      dp_position->set_value(value);
      return true;
    }

    void init(DatapointHandler& handler)
    {
      if (dp_position.has_value())
      {
        dp_position->init(handler);
      }
      if (report_dp_position.has_value())
      {
        report_dp_position->init(handler);
      }
    }

    void dump_config() const
    {
      if (dp_position.has_value())
      {
        ESP_LOGCONFIG(UyatCover::TAG, "   Position is %s", dp_position->get_config().to_string().c_str());
      }
      if (report_dp_position.has_value()) {
        ESP_LOGCONFIG(UyatCover::TAG, "   Position Report is %s", report_dp_position->get_config().to_string().c_str());
      }
    }
  };

  void control(const cover::CoverCall &call) override;
  void apply_direction_();
  cover::CoverTraits get_traits() override;

  Uyat *parent_;
  UyatCoverRestoreMode restore_mode_{};
  std::optional<Control> control_{};
  std::optional<DpSwitch> direction_{};
  Position position_{};
};

}  // namespace uyat
}  // namespace esphome
