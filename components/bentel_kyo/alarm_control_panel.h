/*
 * espkyogate - ESPHome component for Bentel KYO alarms
 * Copyright (C) 2025 Lorenzo De Luca (me@lorenzodeluca.dev)
 * Copyright (C) 2026 Rui Marinho (ruipmarinho@gmail.com)
 *
 * GNU Affero General Public License v3.0
 */

#pragma once

#include "bentel_kyo.h"
#include "esphome/components/alarm_control_panel/alarm_control_panel.h"

namespace esphome {
namespace bentel_kyo {

class BentelKyoAlarmPanel : public alarm_control_panel::AlarmControlPanel, public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_CONNECTION; }

  void set_parent(BentelKyo *parent) { this->parent_ = parent; }
  void set_partition(uint8_t partition) { this->partition_ = partition; }

  // Called by the hub after polling
  void update_state_from_hub();

  // AlarmControlPanel interface
  uint32_t get_supported_features() const override;
  bool get_requires_code() const override { return false; }
  bool get_requires_code_to_arm() const override { return false; }

 protected:
  void control(const alarm_control_panel::AlarmControlPanelCall &call) override;

  BentelKyo *parent_{nullptr};
  uint8_t partition_{1};  // 1-based
};

}  // namespace bentel_kyo
}  // namespace esphome
