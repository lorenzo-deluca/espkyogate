/*
 * espkyogate - ESPHome component for Bentel KYO alarms
 * Copyright (C) 2025 Lorenzo De Luca (me@lorenzodeluca.dev)
 * Copyright (C) 2026 Rui Marinho (ruipmarinho@gmail.com)
 *
 * GNU Affero General Public License v3.0
 */

#include "bentel_kyo.h"
#include "alarm_control_panel.h"

namespace esphome {
namespace bentel_kyo {

// Static constexpr definitions
constexpr uint8_t BentelKyo::CMD_GET_SENSOR_STATUS[];
constexpr uint8_t BentelKyo::CMD_GET_PARTITION_KYO32G[];
constexpr uint8_t BentelKyo::CMD_GET_PARTITION_KYO32[];
constexpr uint8_t BentelKyo::CMD_GET_PARTITION_KYO8[];
constexpr uint8_t BentelKyo::CMD_GET_VERSION[];
constexpr uint8_t BentelKyo::CMD_RESET_ALARMS[];

void BentelKyo::setup() {
  ESP_LOGI(TAG, "Setting up Bentel KYO hub...");
  this->communication_ok_ = false;
  this->force_publish_ = true;
}

void BentelKyo::dump_config() {
  ESP_LOGCONFIG(TAG, "Bentel KYO:");
  if (this->model_detected_) {
    const char *model_name;
    switch (this->alarm_model_) {
      case AlarmModel::KYO_4: model_name = "KYO4"; break;
      case AlarmModel::KYO_8: model_name = "KYO8"; break;
      case AlarmModel::KYO_8G: model_name = "KYO8G"; break;
      case AlarmModel::KYO_8W: model_name = "KYO8W"; break;
      case AlarmModel::KYO_32: model_name = "KYO32"; break;
      case AlarmModel::KYO_32G: model_name = "KYO32G"; break;
      default: model_name = "Unknown"; break;
    }
    ESP_LOGCONFIG(TAG, "  Model: %s", model_name);
    ESP_LOGCONFIG(TAG, "  Firmware: %s", this->firmware_version_);
    ESP_LOGCONFIG(TAG, "  Max Zones: %d", this->max_zones_);
  } else {
    ESP_LOGCONFIG(TAG, "  Model: not yet detected");
  }
  ESP_LOGCONFIG(TAG, "  Alarm panels: %d", this->alarm_panels_.size());
  ESP_LOGCONFIG(TAG, "  Binary sensors: %d", this->binary_sensors_.size());
}

// ========================================
// Async serial: send command (non-blocking)
// ========================================

void BentelKyo::send_command_async_(const uint8_t *cmd, int cmd_len, uint8_t pending_op, uint32_t timeout_ms) {
  // Flush RX buffer
  while (this->available() > 0)
    this->read();

  // Send command bytes (fast: ~7ms for 6 bytes at 9600 baud)
  this->write_array(cmd, cmd_len);

  // Set up async state
  this->serial_state_ = SerialState::WAITING_RESPONSE;
  this->serial_rx_index_ = 0;
  this->serial_cmd_len_ = cmd_len;
  this->serial_sent_ms_ = millis();
  this->serial_last_byte_ms_ = millis();
  this->serial_timeout_ms_ = timeout_ms;
  this->serial_pending_op_ = pending_op;
}

// ========================================
// loop() — non-blocking serial read + response dispatch
// ========================================

void BentelKyo::loop() {
  if (!this->polling_enabled_ || this->serial_state_ != SerialState::WAITING_RESPONSE)
    return;

  // Read any available bytes
  while (this->available() > 0 && this->serial_rx_index_ < 254) {
    this->serial_rx_buf_[this->serial_rx_index_++] = this->read();
    this->serial_last_byte_ms_ = millis();
  }

  // Check for inter-byte silence (response complete)
  bool response_complete = (this->serial_rx_index_ > this->serial_cmd_len_ &&
                            (millis() - this->serial_last_byte_ms_) > INTER_BYTE_SILENCE_MS);

  // Check for timeout (no response or incomplete)
  bool timed_out = (millis() - this->serial_sent_ms_) >= this->serial_timeout_ms_;

  if (!response_complete && !timed_out)
    return;  // Still waiting

  // Response ready or timed out — dispatch
  this->serial_state_ = SerialState::IDLE;
  int count = this->serial_rx_index_;

  if (count <= 0) {
    // No data at all — panel not responding
    ESP_LOGD(TAG, "No answer from serial port (op=%d)", this->serial_pending_op_);
    this->handle_serial_failure_();
    return;
  }

  // Dispatch based on pending operation
  bool ok = false;
  switch (this->serial_pending_op_) {
    case 0:  // detect model
      ok = this->detect_alarm_model_(this->serial_rx_buf_, count);
      if (ok) {
        // Immediately poll sensor+partition status so alarm panels get real state
        // before config reads start (otherwise panels default to DISARMED for ~75s)
        this->send_command_async_(CMD_GET_SENSOR_STATUS, sizeof(CMD_GET_SENSOR_STATUS), 1, 80);
        return;  // Don't update health yet — wait for sensor+partition response
      }
      break;
    case 1:  // sensor status
      ok = this->parse_sensor_status_(this->serial_rx_buf_, count);
      if (ok) {
        // Chain: immediately send partition status query
        const uint8_t *cmd;
        int cmd_len;
        if (this->alarm_model_ == AlarmModel::KYO_32G) {
          cmd = CMD_GET_PARTITION_KYO32G; cmd_len = sizeof(CMD_GET_PARTITION_KYO32G);
        } else if (this->alarm_model_ == AlarmModel::KYO_8 || this->alarm_model_ == AlarmModel::KYO_4 ||
                   this->alarm_model_ == AlarmModel::KYO_8G || this->alarm_model_ == AlarmModel::KYO_8W) {
          cmd = CMD_GET_PARTITION_KYO8; cmd_len = sizeof(CMD_GET_PARTITION_KYO8);
        } else {
          cmd = CMD_GET_PARTITION_KYO32; cmd_len = sizeof(CMD_GET_PARTITION_KYO32);
        }
        this->send_command_async_(cmd, cmd_len, 2, 80);
        return;  // Don't update health yet — wait for partition response
      }
      break;
    case 2:  // partition status
      ok = this->parse_partition_status_(this->serial_rx_buf_, count);
      break;
  }

  // Update communication health
  if (ok) {
    bool was_ok = this->communication_ok_;
    this->communication_ok_ = true;
    this->consecutive_failures_ = 0;
    this->backoff_until_ms_ = 0;
    if (!was_ok) {
      this->force_publish_ = true;
      ESP_LOGI(TAG, "Panel communication restored");
    }
  } else {
    this->handle_serial_failure_();
  }
}

void BentelKyo::handle_serial_failure_() {
  if (this->consecutive_failures_ < 5)
    this->consecutive_failures_++;
  if (this->consecutive_failures_ >= MAX_INVALID_COUNT) {
    this->communication_ok_ = false;
    uint32_t backoff_ms = (1 << this->consecutive_failures_) * 1000UL;
    this->backoff_until_ms_ = millis() + backoff_ms;
    ESP_LOGW(TAG, "Panel not responding, retrying in %lus", backoff_ms / 1000UL);
  }

  // Publish communication status
  for (auto &entry : this->binary_sensors_) {
    if (entry.type == BinarySensorType::COMMUNICATION) {
      entry.sensor->publish_state(this->communication_ok_);
    }
  }
}

// ========================================
// update() — non-blocking: just sends commands, loop() handles responses
// ========================================

void BentelKyo::reread_config() {
  ESP_LOGI(TAG, "Re-reading panel configuration registers...");
  this->config_read_step_ = 0;
  this->esn_read_index_ = 0;
  this->keyfob_read_index_ = 0;
}

void BentelKyo::set_polling_enabled(bool enabled) {
  if (this->polling_enabled_ == enabled)
    return;
  this->polling_enabled_ = enabled;
  if (enabled) {
    ESP_LOGI(TAG, "Polling enabled — resuming panel communication");
    this->force_publish_ = true;
  } else {
    ESP_LOGW(TAG, "Polling disabled — serial communication stopped");
    // Abort any in-progress serial transaction
    this->serial_state_ = SerialState::IDLE;
  }
}

void BentelKyo::update() {
  // Skip if polling is disabled
  if (!this->polling_enabled_)
    return;

  // Skip if still waiting for a response or in backoff
  if (this->serial_state_ != SerialState::IDLE)
    return;
  if (this->backoff_until_ms_ > 0 && millis() < this->backoff_until_ms_)
    return;

  // If model not yet detected, send version query
  if (!this->model_detected_) {
    this->send_command_async_(CMD_GET_VERSION, sizeof(CMD_GET_VERSION), 0, 80);
    return;
  }

  // Read panel configuration once after model detection — one step per update cycle
  // Config reads use blocking send_message_(), so they must NOT run in the same
  // cycle as async sensor/partition polling (otherwise both commands collide on the bus)
  //
  // Steps 3 and 6 (zone ESN and keyfob ESN) read one slot per cycle to avoid
  // blocking the main loop for 90+ seconds (0xC0xx reads take ~1.5s each).
  if (this->config_read_step_ < 9 && this->communication_ok_) {
    switch (this->config_read_step_) {
      case 0: this->config_read_step_ = 1; break;  // skip one cycle after detection
      case 1: this->read_zone_config_(); this->config_read_step_ = 2; break;
      case 2: this->read_zone_names_(); this->config_read_step_ = 3; break;
      case 3:
        // Read one zone ESN per cycle; advance to step 4 when done
        if (this->read_zone_esn_next_())
          this->config_read_step_ = 4;
        break;
      case 4: this->read_output_names_(); this->config_read_step_ = 5; break;
      case 5: this->read_partition_config_(); this->config_read_step_ = 6; break;
      case 6:
        // Read one keyfob ESN per cycle; advance to step 7 when done
        if (this->read_keyfob_esn_next_())
          this->config_read_step_ = 7;
        break;
      case 7: this->read_keyfob_names_(); this->config_read_step_ = 8; break;
      case 8: this->publish_text_sensors_(); this->config_read_step_ = 9; break;
    }
    return;  // Skip normal polling this cycle — avoid bus collision
  }

  // Re-publish text sensors periodically (every 120 polling cycles = ~60s at 500ms)
  // Text sensors are static config data but must be re-published so API clients
  // that connect after initial publish (e.g. Home Assistant reconnects) get the state.
  if (this->config_read_step_ >= 9) {
    this->text_sensor_republish_counter_++;
    if (this->force_publish_ || this->text_sensor_republish_counter_ >= 120) {
      this->publish_text_sensors_();
      this->text_sensor_republish_counter_ = 0;
    }
  }

  // Normal polling: send sensor status query (partition query chains from loop())
  this->send_command_async_(CMD_GET_SENSOR_STATUS, sizeof(CMD_GET_SENSOR_STATUS), 1, 80);

  // Publish communication status
  for (auto &entry : this->binary_sensors_) {
    if (entry.type == BinarySensorType::COMMUNICATION) {
      entry.sensor->publish_state(this->communication_ok_);
    }
  }
}

// ========================================
// Registration methods
// ========================================

void BentelKyo::register_alarm_panel(BentelKyoAlarmPanel *panel) {
  this->alarm_panels_.push_back(panel);
}

void BentelKyo::register_binary_sensor(binary_sensor::BinarySensor *sensor, BinarySensorType type, uint8_t index) {
  this->binary_sensors_.push_back({sensor, type, index});
}

void BentelKyo::register_text_sensor(text_sensor::TextSensor *sensor, TextSensorType type, uint8_t index) {
  this->text_sensors_.push_back({sensor, type, index});
}

// ========================================
// Model detection
// ========================================

bool BentelKyo::detect_alarm_model_(const uint8_t *rx, int count) {
  if (count < RESP_VERSION) {
    ESP_LOGW(TAG, "Version query returned %d bytes", count);
    return false;
  }

  // Extract firmware string from rx[6..17] (12 chars)
  memset(this->firmware_version_, 0, sizeof(this->firmware_version_));
  for (int i = 0; i < 12 && i + 6 < count - 1; i++)
    this->firmware_version_[i] = (char) rx[6 + i];

  ESP_LOGI(TAG, "Firmware: '%s'", this->firmware_version_);

  // Match model from firmware string prefix (longest match first)
  if (strncmp(this->firmware_version_, "KYO32G", 6) == 0) {
    this->alarm_model_ = AlarmModel::KYO_32G;
    this->max_zones_ = KYO_MAX_ZONES;
    ESP_LOGI(TAG, "Detected KYO32G");
  } else if (strncmp(this->firmware_version_, "KYO32", 5) == 0) {
    this->alarm_model_ = AlarmModel::KYO_32;
    this->max_zones_ = KYO_MAX_ZONES;
    ESP_LOGI(TAG, "Detected KYO32");
  } else if (strncmp(this->firmware_version_, "KYO8G", 5) == 0) {
    this->alarm_model_ = AlarmModel::KYO_8G;
    this->max_zones_ = KYO_MAX_ZONES_8;
    ESP_LOGI(TAG, "Detected KYO8G");
  } else if (strncmp(this->firmware_version_, "KYO8W", 5) == 0) {
    this->alarm_model_ = AlarmModel::KYO_8W;
    this->max_zones_ = KYO_MAX_ZONES_8;
    ESP_LOGI(TAG, "Detected KYO8W");
  } else if (strncmp(this->firmware_version_, "KYO8", 4) == 0) {
    this->alarm_model_ = AlarmModel::KYO_8;
    this->max_zones_ = KYO_MAX_ZONES_8;
    ESP_LOGI(TAG, "Detected KYO8");
  } else if (strncmp(this->firmware_version_, "KYO4", 4) == 0) {
    this->alarm_model_ = AlarmModel::KYO_4;
    this->max_zones_ = KYO_MAX_ZONES_8;
    ESP_LOGI(TAG, "Detected KYO4");
  } else {
    ESP_LOGW(TAG, "Unknown model in firmware string '%s'", this->firmware_version_);
    return false;
  }

  this->model_detected_ = true;

  // Publish text sensors
  if (this->firmware_version_sensor_ != nullptr)
    this->firmware_version_sensor_->publish_state(this->firmware_version_);

  if (this->alarm_model_sensor_ != nullptr) {
    const char *model_name;
    switch (this->alarm_model_) {
      case AlarmModel::KYO_4: model_name = "KYO4"; break;
      case AlarmModel::KYO_8: model_name = "KYO8"; break;
      case AlarmModel::KYO_8G: model_name = "KYO8G"; break;
      case AlarmModel::KYO_8W: model_name = "KYO8W"; break;
      case AlarmModel::KYO_32: model_name = "KYO32"; break;
      case AlarmModel::KYO_32G: model_name = "KYO32G"; break;
      default: model_name = "Unknown"; break;
    }
    this->alarm_model_sensor_->publish_state(model_name);
  }

  return true;
}

// ========================================
// Polling
// ========================================

bool BentelKyo::parse_sensor_status_(const uint8_t *rx, int count) {
  bool is_kyo8 = false;
  switch (count) {
    case RESP_SENSOR_KYO32:
      if (!this->model_detected_) {
        this->alarm_model_ = AlarmModel::KYO_32;
        this->max_zones_ = KYO_MAX_ZONES;
        ESP_LOGW(TAG, "Model not detected via firmware, defaulting to KYO32 (18-byte response)");
      }
      break;
    case RESP_SENSOR_KYO8:
      is_kyo8 = true;
      if (!this->model_detected_) {
        this->alarm_model_ = AlarmModel::KYO_8;
        this->max_zones_ = KYO_MAX_ZONES_8;
      }
      break;
    default:
      ESP_LOGE(TAG, "Sensor status: invalid response length %d", count);
      return false;
  }

  is_kyo8 = (this->alarm_model_ == AlarmModel::KYO_8 || this->alarm_model_ == AlarmModel::KYO_4 ||
             this->alarm_model_ == AlarmModel::KYO_8G || this->alarm_model_ == AlarmModel::KYO_8W);

  // Check cache - skip parsing if unchanged
  bool changed = this->force_publish_ || (count != this->sensor_cache_len_) ||
                 (memcmp(rx, this->sensor_cache_, count) != 0);
  memcpy(this->sensor_cache_, rx, count);
  this->sensor_cache_len_ = count;

  if (!changed)
    return true;

  // Parse zone states
  for (int i = 0; i < this->max_zones_; i++) {
    if (is_kyo8) {
      this->zone_state_[i] = (rx[6] >> i) & 1;
    } else {
      this->zone_state_[i] = this->get_zone_bit_32_(rx, 6, i);
    }
  }

  // Parse zone tamper
  for (int i = 0; i < this->max_zones_; i++) {
    if (is_kyo8) {
      this->zone_tamper_[i] = (rx[7] >> i) & 1;
    } else {
      this->zone_tamper_[i] = this->get_zone_bit_32_(rx, 10, i);
    }
  }

  // Parse partition alarm
  for (int i = 0; i < KYO_MAX_PARTITIONS; i++) {
    if (is_kyo8)
      this->partition_alarm_[i] = (rx[9] >> i) & 1;
    else
      this->partition_alarm_[i] = (rx[15] >> i) & 1;
  }

  // Parse warnings
  uint8_t warn_byte = is_kyo8 ? rx[8] : rx[14];
  this->warn_mains_failure_ = (warn_byte >> 0) & 1;
  this->warn_bpi_missing_ = (warn_byte >> 1) & 1;
  this->warn_fuse_fault_ = (warn_byte >> 2) & 1;
  this->warn_low_battery_ = (warn_byte >> 3) & 1;
  if (is_kyo8) {
    this->warn_phone_line_fault_ = (warn_byte >> 5) & 1;
    this->warn_default_codes_ = (warn_byte >> 6) & 1;
    this->warn_wireless_fault_ = false;
  } else {
    this->warn_phone_line_fault_ = (warn_byte >> 4) & 1;
    this->warn_default_codes_ = (warn_byte >> 5) & 1;
    this->warn_wireless_fault_ = (warn_byte >> 6) & 1;
  }

  // Parse tamper/sabotage flags
  uint8_t tamper_byte = is_kyo8 ? rx[10] : rx[16];
  if (is_kyo8) {
    this->tamper_zone_ = (tamper_byte >> 4) & 1;
    this->tamper_false_key_ = (tamper_byte >> 5) & 1;
    this->tamper_bpi_ = (tamper_byte >> 6) & 1;
    this->tamper_system_ = (tamper_byte >> 7) & 1;
    this->tamper_rf_jam_ = false;
    this->tamper_wireless_ = false;
  } else {
    this->tamper_zone_ = (tamper_byte >> 2) & 1;
    this->tamper_false_key_ = (tamper_byte >> 3) & 1;
    this->tamper_bpi_ = (tamper_byte >> 4) & 1;
    this->tamper_system_ = (tamper_byte >> 5) & 1;
    this->tamper_rf_jam_ = (tamper_byte >> 6) & 1;
    this->tamper_wireless_ = (tamper_byte >> 7) & 1;
  }

  this->publish_binary_sensors_();
  return true;
}

bool BentelKyo::parse_partition_status_(const uint8_t *rx, int count) {
  bool is_kyo8 = (this->alarm_model_ == AlarmModel::KYO_8 || this->alarm_model_ == AlarmModel::KYO_4 ||
                  this->alarm_model_ == AlarmModel::KYO_8G || this->alarm_model_ == AlarmModel::KYO_8W);

  if (count != RESP_PARTITION_KYO32 && count != RESP_PARTITION_KYO8) {
    ESP_LOGE(TAG, "Partition status: invalid response length %d", count);
    return false;
  }

  // Check cache
  bool changed = this->force_publish_ || (count != this->partition_cache_len_) ||
                 (memcmp(rx, this->partition_cache_, count) != 0);
  memcpy(this->partition_cache_, rx, count);
  this->partition_cache_len_ = count;

  if (!changed)
    return true;

  // Parse partition arming states (same byte layout for both models)
  for (int i = 0; i < KYO_MAX_PARTITIONS; i++) {
    this->partition_armed_total_[i] = (rx[6] >> i) & 1;
    this->partition_armed_partial_[i] = (rx[7] >> i) & 1;
    this->partition_armed_partial_delay0_[i] = (rx[8] >> i) & 1;
    this->partition_disarmed_[i] = (rx[9] >> i) & 1;
  }

  // Siren status
  this->siren_active_ = (rx[10] >> 5) & 1;

  // Output states (KYO32G only - non-G reads 0xFF)
  if (this->alarm_model_ == AlarmModel::KYO_32G) {
    for (int i = 0; i < KYO_MAX_OUTPUTS; i++)
      this->output_state_[i] = (rx[12] >> i) & 1;
  }

  // Zone bypass, alarm memory, tamper memory
  for (int i = 0; i < this->max_zones_; i++) {
    if (is_kyo8) {
      this->zone_bypass_[i] = (rx[11] >> i) & 1;
      this->zone_alarm_memory_[i] = (rx[12] >> i) & 1;
      this->zone_tamper_memory_[i] = (rx[13] >> i) & 1;
    } else {
      this->zone_bypass_[i] = this->get_zone_bit_32_(rx, 13, i);
      this->zone_alarm_memory_[i] = this->get_zone_bit_32_(rx, 17, i);
      this->zone_tamper_memory_[i] = this->get_zone_bit_32_(rx, 21, i);
    }
  }

  this->force_publish_ = false;
  this->publish_binary_sensors_();
  this->publish_alarm_panels_();
  return true;
}

// ========================================
// State publishing
// ========================================

void BentelKyo::publish_binary_sensors_() {
  for (auto &entry : this->binary_sensors_) {
    bool state = false;
    uint8_t idx = entry.index;

    switch (entry.type) {
      case BinarySensorType::ZONE:
        if (idx < this->max_zones_) state = this->zone_state_[idx];
        break;
      case BinarySensorType::ZONE_TAMPER:
        if (idx < this->max_zones_) state = this->zone_tamper_[idx];
        break;
      case BinarySensorType::ZONE_BYPASS:
        if (idx < this->max_zones_) state = this->zone_bypass_[idx];
        break;
      case BinarySensorType::ZONE_ALARM_MEMORY:
        if (idx < this->max_zones_) state = this->zone_alarm_memory_[idx];
        break;
      case BinarySensorType::ZONE_TAMPER_MEMORY:
        if (idx < this->max_zones_) state = this->zone_tamper_memory_[idx];
        break;
      case BinarySensorType::PARTITION_ALARM:
        if (idx < KYO_MAX_PARTITIONS) state = this->partition_alarm_[idx];
        break;
      case BinarySensorType::WARNING_MAINS_FAILURE:
        state = this->warn_mains_failure_;
        break;
      case BinarySensorType::WARNING_BPI_MISSING:
        state = this->warn_bpi_missing_;
        break;
      case BinarySensorType::WARNING_FUSE_FAULT:
        state = this->warn_fuse_fault_;
        break;
      case BinarySensorType::WARNING_LOW_BATTERY:
        state = this->warn_low_battery_;
        break;
      case BinarySensorType::WARNING_PHONE_LINE_FAULT:
        state = this->warn_phone_line_fault_;
        break;
      case BinarySensorType::WARNING_DEFAULT_CODES:
        state = this->warn_default_codes_;
        break;
      case BinarySensorType::WARNING_WIRELESS_FAULT:
        state = this->warn_wireless_fault_;
        break;
      case BinarySensorType::TAMPER_ZONE:
        state = this->tamper_zone_;
        break;
      case BinarySensorType::TAMPER_FALSE_KEY:
        state = this->tamper_false_key_;
        break;
      case BinarySensorType::TAMPER_BPI:
        state = this->tamper_bpi_;
        break;
      case BinarySensorType::TAMPER_SYSTEM:
        state = this->tamper_system_;
        break;
      case BinarySensorType::TAMPER_RF_JAM:
        state = this->tamper_rf_jam_;
        break;
      case BinarySensorType::TAMPER_WIRELESS:
        state = this->tamper_wireless_;
        break;
      case BinarySensorType::SIREN:
        state = this->siren_active_;
        break;
      case BinarySensorType::OUTPUT_STATE:
        if (idx < KYO_MAX_OUTPUTS) state = this->output_state_[idx];
        break;
      case BinarySensorType::COMMUNICATION:
        // Handled separately in update()
        continue;
    }

    entry.sensor->publish_state(state);
  }
}

void BentelKyo::publish_alarm_panels_() {
  for (auto *panel : this->alarm_panels_) {
    panel->update_state_from_hub();
  }
}

// ========================================
// Commands
// ========================================

void BentelKyo::arm_partition(uint8_t partition, uint8_t arm_type) {
  if (partition < 1 || partition > KYO_MAX_PARTITIONS) {
    ESP_LOGE(TAG, "Invalid partition %d (1-%d)", partition, KYO_MAX_PARTITIONS);
    return;
  }

  ESP_LOGI(TAG, "Arm partition %d type %d", partition, arm_type);
  uint8_t cmd[11] = {0x0F, 0x00, 0xF0, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0xCC, 0xFF};

  // Read current arming state to preserve other partitions
  uint8_t total_mask = 0x00, partial_mask = 0x00;
  for (int i = 0; i < KYO_MAX_PARTITIONS; i++) {
    if (this->partition_armed_total_[i]) total_mask |= (1 << i);
    if (this->partition_armed_partial_[i]) partial_mask |= (1 << i);
  }

  if (arm_type == 2)  // partial
    partial_mask |= 1 << (partition - 1);
  else  // total (arm_type == 1 or 3)
    total_mask |= 1 << (partition - 1);

  cmd[6] = total_mask;
  cmd[7] = partial_mask;
  cmd[9] = calculate_crc_(cmd, 9);

  uint8_t rx[255];
  this->send_message_(cmd, sizeof(cmd), rx, 250);
}

void BentelKyo::disarm_partition(uint8_t partition) {
  if (partition < 1 || partition > KYO_MAX_PARTITIONS) {
    ESP_LOGE(TAG, "Invalid partition %d (1-%d)", partition, KYO_MAX_PARTITIONS);
    return;
  }

  ESP_LOGI(TAG, "Disarm partition %d", partition);
  uint8_t cmd[11] = {0x0F, 0x00, 0xF0, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0xFF, 0xFF};

  // Read current arming state, clear this partition
  uint8_t total_mask = 0x00, partial_mask = 0x00;
  for (int i = 0; i < KYO_MAX_PARTITIONS; i++) {
    if (this->partition_armed_total_[i]) total_mask |= (1 << i);
    if (this->partition_armed_partial_[i]) partial_mask |= (1 << i);
  }

  // Remove this partition from whichever mask it's in
  total_mask &= ~(1 << (partition - 1));
  partial_mask &= ~(1 << (partition - 1));

  cmd[6] = total_mask;
  cmd[7] = partial_mask;
  cmd[9] = calculate_crc_(cmd, 9);

  uint8_t rx[255];
  this->send_message_(cmd, sizeof(cmd), rx, 100);
}

void BentelKyo::reset_alarms() {
  ESP_LOGI(TAG, "Reset alarms");
  uint8_t rx[255];
  this->send_message_(CMD_RESET_ALARMS, sizeof(CMD_RESET_ALARMS), rx, 250);
}

void BentelKyo::activate_output(uint8_t output_number) {
  if (output_number < 1 || output_number > KYO_MAX_OUTPUTS) {
    ESP_LOGE(TAG, "Invalid output %d (1-%d)", output_number, KYO_MAX_OUTPUTS);
    return;
  }

  ESP_LOGI(TAG, "Activate output %d", output_number);
  uint8_t cmd[9] = {0x0F, 0x06, 0xF0, 0x01, 0x00, 0x06, 0x00, 0x00, 0x00};
  cmd[6] = 1 << (output_number - 1);
  cmd[8] = cmd[6];

  uint8_t rx[255];
  this->send_message_(cmd, sizeof(cmd), rx, 250);
}

void BentelKyo::deactivate_output(uint8_t output_number) {
  if (output_number < 1 || output_number > KYO_MAX_OUTPUTS) {
    ESP_LOGE(TAG, "Invalid output %d (1-%d)", output_number, KYO_MAX_OUTPUTS);
    return;
  }

  ESP_LOGI(TAG, "Deactivate output %d", output_number);
  uint8_t cmd[9] = {0x0F, 0x06, 0xF0, 0x01, 0x00, 0x06, 0x00, 0x00, 0xCC};
  cmd[7] = 1 << (output_number - 1);
  cmd[8] = cmd[7];

  uint8_t rx[255];
  this->send_message_(cmd, sizeof(cmd), rx, 250);
}

void BentelKyo::include_zone(uint8_t zone_number) {
  if (zone_number < 1 || zone_number > this->max_zones_) {
    ESP_LOGE(TAG, "Invalid zone %d (1-%d)", zone_number, this->max_zones_);
    return;
  }

  ESP_LOGI(TAG, "Include zone %d", zone_number);
  uint8_t cmd[15] = {0x0F, 0x01, 0xF0, 0x07, 0x00, 0x07,
                     0x00, 0x00, 0x00, 0x00,   // exclude bytes [6-9]
                     0x00, 0x00, 0x00, 0x00,   // include bytes [10-13]
                     0x00};                     // checksum [14]

  // Include bytes are at offsets 10-13: big-endian byte order
  // [10]=zones 25-32, [11]=17-24, [12]=9-16, [13]=1-8
  if (zone_number > 24)
    cmd[10] = 1 << (zone_number - 25);
  else if (zone_number > 16)
    cmd[11] = 1 << (zone_number - 17);
  else if (zone_number > 8)
    cmd[12] = 1 << (zone_number - 9);
  else
    cmd[13] = 1 << (zone_number - 1);

  cmd[14] = calculate_checksum_(cmd, 6, 14);

  uint8_t rx[255];
  this->send_message_(cmd, sizeof(cmd), rx, 250);
}

void BentelKyo::exclude_zone(uint8_t zone_number) {
  if (zone_number < 1 || zone_number > this->max_zones_) {
    ESP_LOGE(TAG, "Invalid zone %d (1-%d)", zone_number, this->max_zones_);
    return;
  }

  ESP_LOGI(TAG, "Exclude zone %d", zone_number);
  uint8_t cmd[15] = {0x0F, 0x01, 0xF0, 0x07, 0x00, 0x07,
                     0x00, 0x00, 0x00, 0x00,   // exclude bytes [6-9]
                     0x00, 0x00, 0x00, 0x00,   // include bytes [10-13]
                     0x00};                     // checksum [14]

  // Exclude bytes are at offsets 6-9: big-endian byte order
  // [6]=zones 25-32, [7]=17-24, [8]=9-16, [9]=1-8
  if (zone_number > 24)
    cmd[6] = 1 << (zone_number - 25);
  else if (zone_number > 16)
    cmd[7] = 1 << (zone_number - 17);
  else if (zone_number > 8)
    cmd[8] = 1 << (zone_number - 9);
  else
    cmd[9] = 1 << (zone_number - 1);

  cmd[14] = calculate_checksum_(cmd, 6, 14);

  uint8_t rx[255];
  this->send_message_(cmd, sizeof(cmd), rx, 250);
}

void BentelKyo::update_datetime(uint8_t day, uint8_t month, uint16_t year,
                                uint8_t hours, uint8_t minutes, uint8_t seconds) {
  if (day == 0 || day > 31 || month == 0 || month > 12 || year < 2000 || year > 2099 ||
      hours > 23 || minutes > 59 || seconds > 59) {
    ESP_LOGE(TAG, "Invalid datetime");
    return;
  }

  ESP_LOGI(TAG, "Update datetime %02d/%02d/%04d %02d:%02d:%02d", day, month, year, hours, minutes, seconds);
  uint8_t cmd[13] = {0x0F, 0x03, 0xF0, 0x05, 0x00, 0x07,
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  cmd[6] = day;
  cmd[7] = month;
  cmd[8] = (uint8_t)(year - 2000);
  cmd[9] = hours;
  cmd[10] = minutes;
  cmd[11] = seconds;
  cmd[12] = calculate_checksum_(cmd, 6, 12);

  uint8_t rx[255];
  this->send_message_(cmd, sizeof(cmd), rx, 300);
}

// ========================================
// Serial I/O
// ========================================

int BentelKyo::send_message_(const uint8_t *cmd, int cmd_len, uint8_t *response, uint32_t timeout_ms) {
  // Flush RX buffer
  while (this->available() > 0)
    this->read();

  // Send command
  this->write_array(cmd, cmd_len);

  // Non-blocking read with inter-byte silence detection
  int index = 0;
  uint8_t rx_buf[255];
  memset(response, 0, 254);

  uint32_t start_ms = millis();
  uint32_t last_byte_ms = start_ms;

  while ((millis() - start_ms) < timeout_ms) {
    if (this->available() > 0) {
      while (this->available() > 0 && index < 254)
        rx_buf[index++] = this->read();
      last_byte_ms = millis();
    } else if (index > cmd_len && (millis() - last_byte_ms) > INTER_BYTE_SILENCE_MS) {
      // Got data beyond echo and silence detected — response complete
      break;
    }
    yield();
  }

  if (index <= 0) {
    ESP_LOGE(TAG, "No answer from serial port");
    return -1;
  }

  // Validate response checksum
  if (index > cmd_len + 1) {
    int data_start = cmd_len;
    int data_end = index - 1;
    uint8_t expected_chk = 0;
    for (int i = data_start; i < data_end; i++)
      expected_chk += rx_buf[i];

    if (expected_chk != rx_buf[data_end])
      ESP_LOGW(TAG, "Response checksum mismatch: expected 0x%02X, got 0x%02X", expected_chk, rx_buf[data_end]);
  }

  memcpy(response, rx_buf, index);
  return index;
}

// ========================================
// Configuration register reads
// ========================================

int BentelKyo::read_register_(uint16_t address, uint8_t length, uint8_t *response, uint32_t timeout_ms) {
  uint8_t cmd[6];
  cmd[0] = 0xF0;
  cmd[1] = address & 0xFF;           // ADDR_LO first (little-endian)
  cmd[2] = (address >> 8) & 0xFF;    // ADDR_HI second
  cmd[3] = length;
  cmd[4] = 0x00;
  cmd[5] = (cmd[0] + cmd[1] + cmd[2] + cmd[3] + cmd[4]) & 0xFF;

  ESP_LOGD(TAG, "Read register 0x%04X len=%d cmd: %02X %02X %02X %02X %02X %02X",
           address, length, cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5]);

  return this->send_message_(cmd, 6, response, timeout_ms);
}

void BentelKyo::read_zone_config_() {
  uint8_t rx[255];

  // Read zones 1-16: address 0x009F, 63 bytes (returns 64 data bytes)
  int count = this->read_register_(0x009F, 0x3F, rx, 300);
  if (count < 6 + 64) {
    ESP_LOGW(TAG, "Zone config read 1-16 failed: got %d bytes", count);
    return;
  }

  for (int i = 0; i < 16 && i < this->max_zones_; i++) {
    int offset = 6 + (i * 4);
    this->zone_type_raw_[i] = rx[offset];
    this->zone_enrolled_[i] = (rx[offset + 1] == 0x01);
    this->zone_area_mask_[i] = rx[offset + 2];
    ESP_LOGD(TAG, "Zone %d raw: [%02X %02X %02X %02X] type=0x%02X enrolled=%d area=0x%02X",
             i + 1, rx[offset], rx[offset + 1], rx[offset + 2], rx[offset + 3],
             this->zone_type_raw_[i], this->zone_enrolled_[i], this->zone_area_mask_[i]);
  }

  // Read zones 17-32 (only for KYO32 models)
  if (this->max_zones_ > 16) {
    count = this->read_register_(0x00DF, 0x3F, rx, 300);
    if (count < 6 + 64) {
      ESP_LOGW(TAG, "Zone config read 17-32 failed: got %d bytes", count);
      return;
    }

    for (int i = 0; i < 16; i++) {
      int offset = 6 + (i * 4);
      this->zone_type_raw_[16 + i] = rx[offset];
      this->zone_enrolled_[16 + i] = (rx[offset + 1] == 0x01);
      this->zone_area_mask_[16 + i] = rx[offset + 2];
    }
  }

  for (int i = 0; i < this->max_zones_; i++) {
    ESP_LOGD(TAG, "Zone %d: type=0x%02X, area=0x%02X, enrolled=%d", i + 1,
             this->zone_type_raw_[i], this->zone_area_mask_[i], this->zone_enrolled_[i]);
  }
}

void BentelKyo::read_zone_names_() {
  // Zone names at 0x2E00-0x2FFF: 16 ASCII bytes per zone, 4 zones per 64-byte read
  static const uint16_t BASE_ADDRS[] = {0x2E00, 0x2E40, 0x2E80, 0x2EC0, 0x2F00, 0x2F40, 0x2F80, 0x2FC0};
  int num_reads = (this->max_zones_ <= 8) ? 2 : 8;

  for (int r = 0; r < num_reads; r++) {
    uint8_t rx[255];
    int count = this->read_register_(BASE_ADDRS[r], 0x3F, rx, 300);
    if (count < 6 + 64) {
      ESP_LOGW(TAG, "Zone names read at 0x%04X failed: got %d bytes", BASE_ADDRS[r], count);
      break;
    }

    for (int n = 0; n < 4; n++) {
      int zone_idx = r * 4 + n;
      if (zone_idx >= this->max_zones_)
        break;

      int offset = 6 + (n * 16);
      char name_buf[17];
      memcpy(name_buf, &rx[offset], 16);
      name_buf[16] = '\0';

      // Trim trailing spaces
      for (int j = 15; j >= 0; j--) {
        if (name_buf[j] == ' ' || name_buf[j] == '\0')
          name_buf[j] = '\0';
        else
          break;
      }

      this->zone_name_[zone_idx] = name_buf;
      ESP_LOGD(TAG, "Zone %d name: '%s'", zone_idx + 1, name_buf);
    }
  }
}

bool BentelKyo::read_zone_esn_next_() {
  // Zone ESN at 0xC045: 3 bytes per zone, per-zone reads with stride 3
  // Reads ONE zone per call (one per update cycle) to avoid blocking the main loop.
  // USB capture shows panel takes ~1s to respond to 0xC0xx reads (EEPROM access).
  // Returns true when all zones have been read.
  int i = this->esn_read_index_;

  if (i >= this->max_zones_) {
    for (int z = 0; z < this->max_zones_; z++) {
      if (this->zone_enrolled_[z])
        ESP_LOGD(TAG, "Zone %d serial: %s", z + 1, this->zone_esn_[z].c_str());
    }
    this->esn_read_index_ = 0;
    return true;
  }

  uint8_t rx[255];
  uint16_t addr = 0xC045 + (i * 3);
  int count = this->read_register_(addr, 0x02, rx, 1500);
  if (count < 6 + 3) {
    ESP_LOGW(TAG, "Zone %d ESN read failed at 0x%04X (%d bytes)", i + 1, addr, count);
    if (i == 0) {
      ESP_LOGW(TAG, "Zone ESN register 0xC045 not available on this panel");
      // Skip all zone ESN reads
      this->esn_read_index_ = 0;
      return true;
    }
    this->esn_read_index_++;
    return false;
  }

  bool is_empty = (rx[6] == 0x00 && rx[7] == 0x00 && rx[8] == 0x00);
  if (is_empty) {
    this->zone_esn_[i] = "Not enrolled";
  } else {
    char sn_buf[12];
    snprintf(sn_buf, sizeof(sn_buf), "%02X%02X%02X", rx[6], rx[7], rx[8]);
    this->zone_esn_[i] = sn_buf;
  }

  this->esn_read_index_++;
  return false;
}

void BentelKyo::read_output_names_() {
  // Output names at 0x3280-0x337F: 16 ASCII bytes per output, 4 per 64-byte read, 16 outputs total
  static const uint16_t BASE_ADDRS[] = {0x3280, 0x32C0, 0x3300, 0x3340};
  int num_reads = 4;  // 16 outputs = 4 reads of 4

  for (int r = 0; r < num_reads; r++) {
    uint8_t rx[255];
    int count = this->read_register_(BASE_ADDRS[r], 0x3F, rx, 300);
    if (count < 6 + 64) {
      ESP_LOGW(TAG, "Output names read at 0x%04X failed: got %d bytes", BASE_ADDRS[r], count);
      break;
    }

    for (int n = 0; n < 4; n++) {
      int out_idx = r * 4 + n;
      if (out_idx >= KYO_MAX_OUTPUTS)
        break;

      int offset = 6 + (n * 16);
      char name_buf[17];
      memcpy(name_buf, &rx[offset], 16);
      name_buf[16] = '\0';

      // Trim trailing spaces
      for (int j = 15; j >= 0; j--) {
        if (name_buf[j] == ' ' || name_buf[j] == '\0')
          name_buf[j] = '\0';
        else
          break;
      }

      this->output_name_[out_idx] = name_buf;
      ESP_LOGD(TAG, "Output %d name: '%s'", out_idx + 1, name_buf);
    }
  }
}

void BentelKyo::read_partition_config_() {
  // Timers at 0x016F: 26 bytes total (section 10.5)
  // Bytes 0-15: entry/exit timers (2 bytes per partition: entry, exit) for 8 partitions
  // Bytes 16-23: siren duration (1 byte per partition)
  uint8_t rx[255];
  int count = this->read_register_(0x016F, 0x1A, rx, 300);
  if (count < 6 + 26) {
    ESP_LOGW(TAG, "Timer register read failed: got %d bytes", count);
    return;
  }

  for (int i = 0; i < KYO_MAX_PARTITIONS; i++) {
    this->partition_entry_delay_[i] = rx[6 + (i * 2)];      // entry delay
    this->partition_exit_delay_[i] = rx[6 + (i * 2) + 1];   // exit delay
    this->partition_siren_timer_[i] = rx[6 + 16 + i];        // siren duration

    if (this->partition_entry_delay_[i] != 0 || this->partition_exit_delay_[i] != 0)
      ESP_LOGD(TAG, "Partition %d: entry=%ds, exit=%ds, siren=%d", i + 1,
               this->partition_entry_delay_[i], this->partition_exit_delay_[i],
               this->partition_siren_timer_[i]);
  }
}

bool BentelKyo::read_keyfob_esn_next_() {
  // Keyfob ESN at 0xC0B1: 3 bytes per keyfob, 16 slots
  // Reads ONE keyfob per call (one per update cycle) to avoid blocking the main loop.
  // Returns true when all keyfobs have been read.
  int i = this->keyfob_read_index_;

  if (i >= KYO_MAX_KEYFOBS) {
    this->keyfob_read_index_ = 0;
    return true;
  }

  uint8_t rx[255];
  uint16_t addr = 0xC0B1 + (i * 3);
  int count = this->read_register_(addr, 0x02, rx, 1500);
  if (count < 6 + 3) {
    if (i == 0) {
      ESP_LOGW(TAG, "Keyfob ESN register 0xC0B1 not available on this panel");
      this->keyfob_read_index_ = 0;
      return true;
    }
    this->keyfob_read_index_++;
    return false;
  }

  bool is_empty = (rx[6] == 0x00 && rx[7] == 0x00 && rx[8] == 0x00);
  if (is_empty) {
    this->keyfob_esn_[i] = "Not enrolled";
  } else {
    char sn_buf[12];
    snprintf(sn_buf, sizeof(sn_buf), "%02X%02X%02X", rx[6], rx[7], rx[8]);
    this->keyfob_esn_[i] = sn_buf;
    ESP_LOGD(TAG, "Keyfob %d serial: %s", i + 1, sn_buf);
  }

  this->keyfob_read_index_++;
  return false;
}

void BentelKyo::read_keyfob_names_() {
  // Keyfob names at 0x3180-0x31FF: 16 ASCII bytes per keyfob, 4 per 64-byte read, 16 keyfobs total
  static const uint16_t BASE_ADDRS[] = {0x3180, 0x31C0, 0x3200, 0x3240};
  int num_reads = 4;  // 16 keyfobs = 4 reads of 4

  for (int r = 0; r < num_reads; r++) {
    uint8_t rx[255];
    int count = this->read_register_(BASE_ADDRS[r], 0x3F, rx, 300);
    if (count < 6 + 64) {
      ESP_LOGW(TAG, "Keyfob names read at 0x%04X failed: got %d bytes", BASE_ADDRS[r], count);
      break;
    }

    for (int n = 0; n < 4; n++) {
      int kf_idx = r * 4 + n;
      if (kf_idx >= KYO_MAX_KEYFOBS)
        break;

      int offset = 6 + (n * 16);
      char name_buf[17];
      memcpy(name_buf, &rx[offset], 16);
      name_buf[16] = '\0';

      // Trim trailing spaces
      for (int j = 15; j >= 0; j--) {
        if (name_buf[j] == ' ' || name_buf[j] == '\0')
          name_buf[j] = '\0';
        else
          break;
      }

      this->keyfob_name_[kf_idx] = name_buf;
      ESP_LOGD(TAG, "Keyfob %d name: '%s'", kf_idx + 1, name_buf);
    }
  }
}

void BentelKyo::publish_text_sensors_() {
  for (auto &entry : this->text_sensors_) {
    uint8_t idx = entry.index;

    switch (entry.type) {
      case TEXT_ZONE_TYPE: {
        if (idx >= (uint8_t) this->max_zones_) continue;
        const char *type_str;
        switch (this->zone_type_raw_[idx]) {
          case 0x00: type_str = "Instant"; break;
          case 0x01: type_str = "Delayed"; break;
          case 0x02: type_str = "Path"; break;
          case 0x18: type_str = "Unconfigured"; break;
          default: type_str = "Unknown"; break;
        }
        entry.sensor->publish_state(type_str);
        break;
      }
      case TEXT_ZONE_NAME:
        if (idx >= (uint8_t) this->max_zones_) continue;
        entry.sensor->publish_state(this->zone_name_[idx]);
        break;
      case TEXT_ZONE_AREA: {
        if (idx >= (uint8_t) this->max_zones_) continue;
        std::string partitions;
        for (int bit = 0; bit < 8; bit++) {
          if (this->zone_area_mask_[idx] & (1 << bit)) {
            if (!partitions.empty())
              partitions += ", ";
            partitions += to_string(bit + 1);
          }
        }
        if (partitions.empty())
          partitions = "None";
        entry.sensor->publish_state(partitions);
        break;
      }
      case TEXT_ZONE_ESN:
        if (idx >= (uint8_t) this->max_zones_) continue;
        entry.sensor->publish_state(this->zone_esn_[idx].empty() ? "N/A" : this->zone_esn_[idx]);
        break;
      case TEXT_OUTPUT_NAME:
        if (idx >= KYO_MAX_OUTPUTS) continue;
        entry.sensor->publish_state(this->output_name_[idx]);
        break;
      case TEXT_KEYFOB_ESN:
        if (idx >= KYO_MAX_KEYFOBS) continue;
        entry.sensor->publish_state(this->keyfob_esn_[idx].empty() ? "N/A" : this->keyfob_esn_[idx]);
        break;
      case TEXT_KEYFOB_NAME:
        if (idx >= KYO_MAX_KEYFOBS) continue;
        entry.sensor->publish_state(this->keyfob_name_[idx].empty() ? "N/A" : this->keyfob_name_[idx]);
        break;
      case TEXT_PARTITION_ENTRY_DELAY:
        if (idx >= KYO_MAX_PARTITIONS) continue;
        entry.sensor->publish_state(to_string(this->partition_entry_delay_[idx]) + "s");
        break;
      case TEXT_PARTITION_EXIT_DELAY:
        if (idx >= KYO_MAX_PARTITIONS) continue;
        entry.sensor->publish_state(to_string(this->partition_exit_delay_[idx]) + "s");
        break;
      case TEXT_PARTITION_SIREN_TIMER:
        if (idx >= KYO_MAX_PARTITIONS) continue;
        entry.sensor->publish_state(to_string(this->partition_siren_timer_[idx]));
        break;
    }
  }
}

// ========================================
// Helpers
// ========================================

uint8_t BentelKyo::calculate_crc_(const uint8_t *cmd, int len) {
  int sum = 0;
  for (int i = 0; i < len; i++)
    sum += cmd[i];
  return (uint8_t)(0x203 - sum);
}

uint8_t BentelKyo::calculate_checksum_(const uint8_t *data, int offset, int len) {
  uint8_t cksum = 0;
  for (int i = offset; i < len; i++)
    cksum += data[i];
  return cksum;
}

bool BentelKyo::get_zone_bit_32_(const uint8_t *rx, int base_offset, int zone_index) {
  // KYO32 zone layout: big-endian byte order
  // base+0 = zones 25-32, base+1 = 17-24, base+2 = 9-16, base+3 = 1-8
  if (zone_index >= 24)
    return (rx[base_offset] >> (zone_index - 24)) & 1;
  else if (zone_index >= 16)
    return (rx[base_offset + 1] >> (zone_index - 16)) & 1;
  else if (zone_index >= 8)
    return (rx[base_offset + 2] >> (zone_index - 8)) & 1;
  else
    return (rx[base_offset + 3] >> zone_index) & 1;
}

}  // namespace bentel_kyo
}  // namespace esphome
