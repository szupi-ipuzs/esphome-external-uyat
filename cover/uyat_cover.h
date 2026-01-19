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

class UyatCover : public cover::Cover, public Component {
 public:

  struct ControlDpValueMapping
  {
    std::optional<uint32_t> open_value;
    std::optional<uint32_t> close_value;
    std::optional<uint32_t> stop_value;
  };

  struct ConfigControl
  {
    MatchingDatapoint matching_dp;
    ControlDpValueMapping mapping;
  };

  struct DirectionConfig
  {
    MatchingDatapoint matching_dp;
    bool inverted;
  };

  struct PositionConfig
  {
    std::optional<MatchingDatapoint> position_dp;
    std::optional<MatchingDatapoint> position_report_dp;
    bool inverted;
    uint32_t min_value;
    uint32_t max_value;
    std::optional<uint32_t> uncalibrated_value;
  };

  struct Config
  {
    PositionConfig position_config;
    std::optional<ConfigControl> control_config;
    std::optional<DirectionConfig> direction_config;
    UyatCoverRestoreMode restore_mode;
  };

  explicit UyatCover(Uyat *parent, Config config);

  void setup() override;
  void dump_config() override;

 private:

  static constexpr const char* TAG = "uyat.cover";

  struct ControlHandler
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

  struct PositionHandler
  {
    PositionHandler(PositionConfig&& config, DpDimmer::BrightnessChangedCallback on_position_lambda)
    {
      if (config.position_report_dp)
      {
        report_dp_position.emplace(on_position_lambda,
                  std::move(*config.position_report_dp),
                  config.min_value, config.max_value, config.inverted);
        on_position_lambda = [](const float){};
      }

      if (config.position_dp)
      {
        dp_position.emplace(
                    on_position_lambda,
                    std::move(*config.position_dp),
                    config.min_value, config.max_value, config.inverted);
      }

      uncalibrated_value = config.uncalibrated_value;
    }

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
  cover::CoverTraits get_traits() override;

  void configure_control(ConfigControl&& config);
  void configure_direction(DirectionConfig&& config);
  void configure_position(PositionConfig&& config);
  void apply_direction_();

  void on_position_value(const float);

  Uyat& parent_;
  UyatCoverRestoreMode restore_mode_{};
  std::optional<ControlHandler> control_{};
  std::optional<DpSwitch> direction_{};
  PositionHandler position_;
};

}  // namespace uyat
}  // namespace esphome
