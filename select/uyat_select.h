#pragma once

#include "esphome/core/component.h"
#include "esphome/components/select/select.h"

#include "../uyat.h"
#include "../dp_number.h"

#include <optional>
#include <vector>

namespace esphome {
namespace uyat {

class UyatSelect : public select::Select, public Component {
 private:
  void on_value(const float);
  std::string get_object_id() const;
  std::optional<std::size_t> translate(const uint32_t number_value) const;
 public:
  void setup() override;
  void dump_config() override;

  void configure(MatchingDatapoint select_dp){
    this->dp_number_.emplace([this](const float value){this->on_value(value);},
                             std::move(select_dp),
                             0, 1.0f);
  }

  void set_uyat_parent(Uyat *parent) { this->parent_ = parent; }
  void set_optimistic(bool optimistic) { this->optimistic_ = optimistic; }
  void set_select_mappings(std::vector<uint32_t> mappings) { this->mappings_ = std::move(mappings); }

 protected:
  void control(size_t index) override;

  Uyat *parent_;
  bool optimistic_ = false;
  std::vector<uint32_t> mappings_;
  std::optional<DpNumber> dp_number_;
};

}  // namespace uyat
}  // namespace esphome
