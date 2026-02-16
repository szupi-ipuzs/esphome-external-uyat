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

  static constexpr const char* TAG = "uyat.select";

  void on_value(const float);
  std::optional<std::size_t> translate(const uint32_t number_value) const;
 public:

  struct Config
  {
    MatchingDatapoint matching_dp;
    bool optimistic;
    std::vector<uint32_t> mappings;
  };

  explicit UyatSelect(Uyat *parent, Config config);

  void setup() override;
  void dump_config() override;

 protected:
  void control(size_t index) override;

  Uyat& parent_;
  bool optimistic_;
  std::vector<uint32_t> mappings_;
  DpNumber dp_number_;
};

}  // namespace uyat
}  // namespace esphome
