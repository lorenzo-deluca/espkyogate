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

class BentelKyoArmAllButton : public button::Button, public Component {
 public:
  void set_parent(BentelKyo *parent) { this->parent_ = parent; }
  void set_arm_type(uint8_t arm_type) { this->arm_type_ = arm_type; }

  void press_action() override {
    this->parent_->arm_all_partitions(this->arm_type_);
  }

 protected:
  BentelKyo *parent_{nullptr};
  uint8_t arm_type_{1};  // 1=total, 2=partial, 3=partial delay 0
};

class BentelKyoDisarmAllButton : public button::Button, public Component {
 public:
  void set_parent(BentelKyo *parent) { this->parent_ = parent; }

  void press_action() override {
    this->parent_->disarm_all_partitions();
  }

 protected:
  BentelKyo *parent_{nullptr};
};

class BentelKyoArmPresetButton : public button::Button, public Component {
 public:
  void set_parent(BentelKyo *parent) { this->parent_ = parent; }
  void set_masks(uint8_t total, uint8_t partial, uint8_t partial_d0, uint8_t configured) {
    this->total_mask_ = total;
    this->partial_mask_ = partial;
    this->partial_d0_mask_ = partial_d0;
    this->configured_mask_ = configured;
  }

  void press_action() override {
    this->parent_->arm_preset(this->total_mask_, this->partial_mask_,
                              this->partial_d0_mask_, this->configured_mask_);
  }

 protected:
  BentelKyo *parent_{nullptr};
  uint8_t total_mask_{0};
  uint8_t partial_mask_{0};
  uint8_t partial_d0_mask_{0};
  uint8_t configured_mask_{0};
};

}  // namespace bentel_kyo
}  // namespace esphome
