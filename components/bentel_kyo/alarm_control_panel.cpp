/*
 * espkyogate - ESPHome component for Bentel KYO alarms
 * Copyright (C) 2025 Lorenzo De Luca (me@lorenzodeluca.dev)
 * Copyright (C) 2026 Rui Marinho (ruipmarinho@gmail.com)
 *
 * GNU Affero General Public License v3.0
 */

#include "alarm_control_panel.h"

namespace esphome {
namespace bentel_kyo {

static const char *const TAG_ACP = "bentel_kyo.alarm_panel";

void BentelKyoAlarmPanel::setup() {
  ESP_LOGD(TAG_ACP, "Setting up alarm panel for partition %d", this->partition_);
}

void BentelKyoAlarmPanel::dump_config() {
  ESP_LOGCONFIG(TAG_ACP, "Bentel KYO Alarm Panel:");
  ESP_LOGCONFIG(TAG_ACP, "  Partition: %d", this->partition_);
}

uint32_t BentelKyoAlarmPanel::get_supported_features() const {
  return alarm_control_panel::ACP_FEAT_ARM_HOME |
         alarm_control_panel::ACP_FEAT_ARM_AWAY |
         alarm_control_panel::ACP_FEAT_ARM_NIGHT |
         alarm_control_panel::ACP_FEAT_TRIGGER;
}

bool BentelKyoAlarmPanel::is_code_valid_(optional<std::string> code) const {
  if (this->codes_.empty())
    return true;
  if (!code.has_value() || code.value().empty())
    return false;
  for (const auto &c : this->codes_) {
    if (c == code.value())
      return true;
  }
  return false;
}

void BentelKyoAlarmPanel::control(const alarm_control_panel::AlarmControlPanelCall &call) {
  if (!call.get_state().has_value())
    return;

  auto state = *call.get_state();
  auto code = call.get_code();

  // Validate code for disarm (always required if codes are configured)
  if (state == alarm_control_panel::ACP_STATE_DISARMED && !this->is_code_valid_(code)) {
    ESP_LOGW(TAG_ACP, "Invalid code for disarm on partition %d", this->partition_);
    return;
  }

  // Validate code for arm (only if requires_code_to_arm is set)
  if (state != alarm_control_panel::ACP_STATE_DISARMED &&
      state != alarm_control_panel::ACP_STATE_TRIGGERED &&
      this->requires_code_to_arm_ && !this->is_code_valid_(code)) {
    ESP_LOGW(TAG_ACP, "Invalid code for arm on partition %d", this->partition_);
    return;
  }

  switch (state) {
    case alarm_control_panel::ACP_STATE_ARMED_AWAY:
      // Total arm
      this->parent_->arm_partition(this->partition_, 1);
      break;
    case alarm_control_panel::ACP_STATE_ARMED_HOME:
      // Partial arm
      this->parent_->arm_partition(this->partition_, 2);
      break;
    case alarm_control_panel::ACP_STATE_ARMED_NIGHT:
      // Partial arm with delay 0
      this->parent_->arm_partition(this->partition_, 3);
      break;
    case alarm_control_panel::ACP_STATE_DISARMED:
      this->parent_->disarm_partition(this->partition_);
      break;
    case alarm_control_panel::ACP_STATE_TRIGGERED:
      this->publish_state(alarm_control_panel::ACP_STATE_TRIGGERED);
      break;
    default:
      break;
  }
}

void BentelKyoAlarmPanel::update_state_from_hub() {
  if (this->parent_ == nullptr)
    return;

  uint8_t idx = this->partition_ - 1;  // Convert to 0-based

  // Map KYO state to ESPHome alarm_control_panel state
  // Priority: triggered > armed_away > armed_home > armed_night > disarmed
  alarm_control_panel::AlarmControlPanelState new_state;

  if (this->parent_->partition_alarm_[idx]) {
    new_state = alarm_control_panel::ACP_STATE_TRIGGERED;
  } else if (this->parent_->partition_armed_total_[idx]) {
    new_state = alarm_control_panel::ACP_STATE_ARMED_AWAY;
  } else if (this->parent_->partition_armed_partial_[idx]) {
    new_state = alarm_control_panel::ACP_STATE_ARMED_HOME;
  } else if (this->parent_->partition_armed_partial_delay0_[idx]) {
    new_state = alarm_control_panel::ACP_STATE_ARMED_NIGHT;
  } else if (this->parent_->partition_disarmed_[idx]) {
    new_state = alarm_control_panel::ACP_STATE_DISARMED;
  } else {
    // No state bits set — keep current state
    return;
  }

  if (new_state != this->get_state()) {
    this->publish_state(new_state);
  }
}

}  // namespace bentel_kyo
}  // namespace esphome
