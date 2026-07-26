/*
 * espkyogate - ESPHome native component for Bentel KYO Units
 * Project started and maintained by Lorenzo De Luca (me@lorenzodeluca.dev)
 * Special thanks for ESPHome native component refactor to Rui Marinho (ruipmarinho@gmail.com)
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

class BentelKyoResetAlarmsButton : public button::Button, public Component {
 public:
  void set_parent(BentelKyo *parent) { this->parent_ = parent; }

  void press_action() override {
    this->parent_->reset_alarms();
  }

 protected:
  BentelKyo *parent_{nullptr};
};

class BentelKyoReadEventLogButton : public button::Button, public Component {
 public:
  void set_parent(BentelKyo *parent) { this->parent_ = parent; }

  void press_action() override {
    this->parent_->read_event_log();
  }

 protected:
  BentelKyo *parent_{nullptr};
};

class BentelKyoMemoryScanButton : public button::Button, public Component {
 public:
  void set_parent(BentelKyo *parent) { this->parent_ = parent; }

  void press_action() override {
    this->parent_->memory_scan();
  }

 protected:
  BentelKyo *parent_{nullptr};
};

class BentelKyoDumpConfigButton : public button::Button, public Component {
 public:
  void set_parent(BentelKyo *parent) { this->parent_ = parent; }

  void press_action() override {
    this->parent_->dump_config();
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
  void set_masks(uint8_t total, uint8_t partial, uint8_t partial_d0) {
    this->total_mask_ = total;
    this->partial_mask_ = partial;
    this->partial_d0_mask_ = partial_d0;
  }

  void press_action() override {
    this->parent_->arm_preset(this->total_mask_, this->partial_mask_,
                              this->partial_d0_mask_);
  }

 protected:
  BentelKyo *parent_{nullptr};
  uint8_t total_mask_{0};
  uint8_t partial_mask_{0};
  uint8_t partial_d0_mask_{0};
};

class BentelKyoActivateOutputButton : public button::Button, public Component {
 public:
  void set_parent(BentelKyo *parent) { this->parent_ = parent; }
  void set_output_number(uint8_t output_number) { this->output_number_ = output_number; }

  void press_action() override {
    this->parent_->activate_output(this->output_number_);
  }

 protected:
  BentelKyo *parent_{nullptr};
  uint8_t output_number_{1};
};

class BentelKyoDeactivateOutputButton : public button::Button, public Component {
 public:
  void set_parent(BentelKyo *parent) { this->parent_ = parent; }
  void set_output_number(uint8_t output_number) { this->output_number_ = output_number; }

  void press_action() override {
    this->parent_->deactivate_output(this->output_number_);
  }

 protected:
  BentelKyo *parent_{nullptr};
  uint8_t output_number_{1};
};

class BentelKyoPulseOutputButton : public button::Button, public Component {
 public:
  void set_parent(BentelKyo *parent) { this->parent_ = parent; }
  void set_output_number(uint8_t output_number) { this->output_number_ = output_number; }
  void set_pulse_time(uint32_t pulse_time_ms) { this->pulse_time_ms_ = pulse_time_ms; }

  void press_action() override {
    this->parent_->pulse_output(this->output_number_, this->pulse_time_ms_);
  }

 protected:
  BentelKyo *parent_{nullptr};
  uint8_t output_number_{1};
  uint32_t pulse_time_ms_{1000};  // default 1 second
};

class BentelKyoIncludeZoneButton : public button::Button, public Component {
 public:
  void set_parent(BentelKyo *parent) { this->parent_ = parent; }
  void set_zone_number(uint8_t zone_number) { this->zone_number_ = zone_number; }

  void press_action() override {
    this->parent_->include_zone(this->zone_number_);
  }

 protected:
  BentelKyo *parent_{nullptr};
  uint8_t zone_number_{1};
};

class BentelKyoExcludeZoneButton : public button::Button, public Component {
 public:
  void set_parent(BentelKyo *parent) { this->parent_ = parent; }
  void set_zone_number(uint8_t zone_number) { this->zone_number_ = zone_number; }

  void press_action() override {
    this->parent_->exclude_zone(this->zone_number_);
  }

 protected:
  BentelKyo *parent_{nullptr};
  uint8_t zone_number_{1};
};

class BentelKyoSyncDatetimeButton : public button::Button, public Component {
 public:
  void set_parent(BentelKyo *parent) { this->parent_ = parent; }

  void press_action() override {
    this->parent_->sync_datetime_from_ntp();
  }

 protected:
  BentelKyo *parent_{nullptr};
};

}  // namespace bentel_kyo
}  // namespace esphome
