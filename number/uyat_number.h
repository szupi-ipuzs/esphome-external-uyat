#pragma once

#include "esphome/components/number/number.h"
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"

#include "../uyat.h"
#include "../dp_number.h"

namespace esphome {
namespace uyat {

class UyatNumber : public number::Number, public Component {
 private:
  void on_value(const float);
  std::string get_object_id() const;

 public:
  void setup() override;
  void dump_config() override;
  void configure(MatchingDatapoint number_dp, const float offset = 0.0f, const float multiplier = 1.0f){
    this->dp_number_.emplace([this](const float value){this->on_value(value);},
                             std::move(number_dp),
                             offset, multiplier);
  }

  void set_uyat_parent(Uyat *parent) { this->parent_ = parent; }

 protected:
  void control(float value) override;

  Uyat *parent_;
  std::optional<DpNumber> dp_number_;
};

}  // namespace uyat
}  // namespace esphome
