/*
 * espkyogate - ESPHome component for Bentel KYO alarms
 * Copyright (C) 2025 Lorenzo De Luca (me@lorenzodeluca.dev)
 * Copyright (C) 2026 Rui Marinho (ruipmarinho@gmail.com)
 *
 * GNU Affero General Public License v3.0
 */

#pragma once

#include "esphome/components/switch/switch.h"
#include "bentel_kyo.h"

namespace esphome {
namespace bentel_kyo {

class BentelKyoPollingSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(BentelKyo *parent) { this->parent_ = parent; }

  void setup() override {
    this->publish_state(this->parent_->is_polling_enabled());
  }

  void write_state(bool state) override {
    this->parent_->set_polling_enabled(state);
    this->publish_state(state);
  }

 protected:
  BentelKyo *parent_{nullptr};
};

}  // namespace bentel_kyo
}  // namespace esphome
