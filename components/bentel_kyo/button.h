/*
 * espkyogate - ESPHome component for Bentel KYO alarms
 * Copyright (C) 2025 Lorenzo De Luca (me@lorenzodeluca.dev)
 * Copyright (C) 2026 Rui Marinho (ruipmarinho@gmail.com)
 *
 * GNU Affero General Public License v3.0
 */

#pragma once

#include "esphome/components/button/button.h"
#include "bentel_kyo.h"

namespace esphome {
namespace bentel_kyo {

class BentelKyoRereadConfigButton : public button::Button, public Component {
 public:
  void set_parent(BentelKyo *parent) { this->parent_ = parent; }

  void press_action() override {
    this->parent_->reread_config();
  }

 protected:
  BentelKyo *parent_{nullptr};
};

}  // namespace bentel_kyo
}  // namespace esphome
