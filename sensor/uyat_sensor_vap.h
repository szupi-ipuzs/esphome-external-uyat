#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"

#include "../uyat.h"
#include "../dp_vap.h"

#include <optional>

namespace esphome::uyat
{

enum class UyatVAPValueType {
  VOLTAGE,
  AMPERAGE,
  POWER,
};

class UyatSensorVAP : public sensor::Sensor, public Component {
 private:
  void on_value(const DpVAP::VAPValue&);
  std::string get_object_id() const;

 public:

  void setup() override;
  void dump_config() override;
  void configure(MatchingDatapoint vap_dp, const UyatVAPValueType value_type){
    this->value_type_ = value_type;
    this->dp_vap_.emplace([this](const DpVAP::VAPValue& value){this->on_value(value);},
                             std::move(vap_dp));
  }

  void set_uyat_parent(Uyat *parent) { this->parent_ = parent; }

 protected:
  Uyat *parent_;
  std::optional<DpVAP> dp_vap_;
  UyatVAPValueType value_type_{UyatVAPValueType::VOLTAGE};
};

}  // namespace
