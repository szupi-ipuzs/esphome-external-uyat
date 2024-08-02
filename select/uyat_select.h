#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uyat/uyat.h"
#include "esphome/components/select/select.h"

#include <vector>

namespace esphome {
namespace uyat {

class UyatSelect : public select::Select, public Component {
 public:
  void setup() override;
  void dump_config() override;

  void set_uyat_parent(Uyat *parent) { this->parent_ = parent; }
  void set_optimistic(bool optimistic) { this->optimistic_ = optimistic; }
  void set_select_id(uint8_t select_id) { this->select_id_ = select_id; }
  void set_select_mappings(std::vector<uint8_t> mappings) { this->mappings_ = std::move(mappings); }

 protected:
  void control(const std::string &value) override;

  Uyat *parent_;
  bool optimistic_ = false;
  uint8_t select_id_;
  std::vector<uint8_t> mappings_;
};

}  // namespace uyat
}  // namespace esphome
