#pragma once

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"

#include "../uyat.h"
#include "../dp_number.h"

namespace esphome {
namespace uyat {

class UyatTextSensorMapped : public text_sensor::TextSensor, public Component {
 private:
  void on_value(const float);
  std::string get_object_id() const;
  std::string translate(const uint32_t number_value) const;

 public:
  void setup() override;
  void dump_config() override;
  void configure(MatchingDatapoint number_dp){
    this->dp_number_.emplace([this](const float value){
      this->on_value(value);
    },
    std::move(number_dp),
    0, 1.0f);
  }
  void set_mappings(std::vector<std::pair<uint32_t, std::string>> mappings) { this->mappings_ = std::move(mappings); }
  void add_mapping(const uint32_t number_value, const std::string &text_value) { this->mappings_.emplace_back(number_value, text_value); }

  void set_uyat_parent(Uyat *parent) { this->parent_ = parent; }

 protected:
  Uyat *parent_;
  std::optional<DpNumber> dp_number_;
  std::vector<std::pair<uint32_t, std::string>> mappings_;
};

}  // namespace uyat
}  // namespace esphome
