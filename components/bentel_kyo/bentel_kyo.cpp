/*
 * espkyogate - ESPHome native component for Bentel KYO Units
 * Project started and maintained by Lorenzo De Luca (me@lorenzodeluca.dev)
 * Special thanks for ESPHome native component refactor to Rui Marinho (ruipmarinho@gmail.com)
 *
 * GNU Affero General Public License v3.0
 */

#include "bentel_kyo.h"
#include "alarm_control_panel.h"
#include "cp437.h"
#include <ctime>

namespace esphome {
namespace bentel_kyo {

// Static constexpr definitions
constexpr uint8_t BentelKyo::CMD_GET_SENSOR_STATUS[];
constexpr uint8_t BentelKyo::CMD_GET_PARTITION_KYO32[];
constexpr uint8_t BentelKyo::CMD_GET_PARTITION_KYO32G[];
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

  // Log serial trace if enabled
  if (this->serial_trace_) {
    ESP_LOGI(TAG, "TX [%d bytes]: %s", cmd_len,
             format_hex_pretty(cmd, cmd_len).c_str());
  }

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
  // Handle non-blocking pulse output timer
  for (int i = 0; i < KYO_MAX_OUTPUTS; i++) {
    if (this->pulse_output_end_ms_[i] > 0 && millis() >= this->pulse_output_end_ms_[i]) {
      this->pulse_output_end_ms_[i] = 0;
      this->deactivate_output(i + 1);
    }
  }

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

  // Log serial trace if enabled
  if (this->serial_trace_ && count > 0) {
    ESP_LOGI(TAG, "RX [%d bytes]: %s", count,
             format_hex_pretty(this->serial_rx_buf_, count).c_str());
  }

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
        this->send_command_async_(CMD_GET_SENSOR_STATUS, sizeof(CMD_GET_SENSOR_STATUS), 1);
        return;  // Don't update health yet — wait for sensor+partition response
      }
      break;
    case 1:  // sensor status
      ok = this->parse_sensor_status_(this->serial_rx_buf_, count);
      if (ok) {
        // Chain: immediately send partition status query
        const uint8_t *cmd;
        int cmd_len;
        if (this->alarm_model_ == AlarmModel::KYO_8 || this->alarm_model_ == AlarmModel::KYO_4 ||
            this->alarm_model_ == AlarmModel::KYO_8G) {
          cmd = CMD_GET_PARTITION_KYO8; cmd_len = sizeof(CMD_GET_PARTITION_KYO8);
        } else if (this->alarm_model_ == AlarmModel::KYO_32) {
          // KYO32 non-G reads partition status at 0x14EC. Using the KYO32G 0x1502
          // register here returns FF padding, which the parser reads as all-disarmed
          // (issue #118, regression from unifying the two commands). KYO8W is NOT in
          // this branch: it was hardware-confirmed on the 0x1502 register in #109.
          //
          // Some non-G panels are the other way around (issue #122): 0x14EC comes back
          // completely unmapped and 0x1502 is the one that works. partition_addr_use_alt_
          // is learned at runtime in the case 2 handler below when that's detected.
          if (this->partition_addr_use_alt_) {
            cmd = CMD_GET_PARTITION_KYO32G; cmd_len = sizeof(CMD_GET_PARTITION_KYO32G);
          } else {
            cmd = CMD_GET_PARTITION_KYO32; cmd_len = sizeof(CMD_GET_PARTITION_KYO32);
          }
        } else {
          // KYO32G and KYO8W both read partition status at 0x1502.
          cmd = CMD_GET_PARTITION_KYO32G; cmd_len = sizeof(CMD_GET_PARTITION_KYO32G);
        }
        this->send_command_async_(cmd, cmd_len, 2);
        return;  // Don't update health yet — wait for partition response
      }
      break;
    case 2:  // partition status
      if (this->alarm_model_ == AlarmModel::KYO_32 && !this->partition_addr_fallback_tried_ &&
          this->partition_status_looks_unmapped_(this->serial_rx_buf_, count)) {
        // 0x14EC returned no partition data at all — this panel needs the KYO32G
        // register instead (issue #122). Switch and retry immediately, once.
        this->partition_addr_fallback_tried_ = true;
        this->partition_addr_use_alt_ = true;
        ESP_LOGW(TAG, "Partition status at 0x14EC returned no partition data — retrying "
                      "with the KYO32G register (0x1502), which this panel appears to need");
        this->send_command_async_(CMD_GET_PARTITION_KYO32G, sizeof(CMD_GET_PARTITION_KYO32G), 2);
        return;
      }
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
  if (this->consecutive_failures_ < 7)
    this->consecutive_failures_++;
  if (this->consecutive_failures_ >= MAX_INVALID_COUNT) {
    bool was_ok = this->communication_ok_;
    this->communication_ok_ = false;
    uint32_t backoff_ms = (1UL << (this->consecutive_failures_ - (MAX_INVALID_COUNT - 1))) * 1000UL;
    this->backoff_until_ms_ = millis() + backoff_ms;
    ESP_LOGW(TAG, "Panel not responding, retrying in %lus", backoff_ms / 1000UL);

    // Transition to communication lost: invalidate all sensor states
    if (was_ok) {
      ESP_LOGW(TAG, "Communication lost — setting all sensors to unknown");

      // Invalidate all binary sensors (except COMMUNICATION itself)
      for (auto &entry : this->binary_sensors_) {
        if (entry.type != BinarySensorType::COMMUNICATION) {
          entry.sensor->invalidate_state();
        }
      }

      // Invalidate alarm panel states
      for (auto *panel : this->alarm_panels_) {
        panel->invalidate_state();
      }

      // Clear response caches so states are re-published when communication resumes
      this->sensor_cache_len_ = 0;
      this->partition_cache_len_ = 0;
      this->force_publish_ = true;
    }
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
  this->config_chunk_index_ = 0;
  this->esn_read_index_ = 0;
  this->keyfob_read_index_ = 0;
}

void BentelKyo::read_event_log() {
  ESP_LOGI(TAG, "Event log dump requested — reading 28 chunks...");
  this->event_log_read_pending_ = true;
  this->event_log_chunk_index_ = 0;
  this->event_log_entries_logged_ = 0;
}

void BentelKyo::memory_scan() {
  ESP_LOGI(TAG, "Memory scan requested — dumping unmapped config regions (issue #113)...");
  ESP_LOGW(TAG, "Scan output contains personal data (event log, code/phone names, access PINs) — redact before sharing");
  this->memory_scan_pending_ = true;
  this->memory_scan_chunk_index_ = 0;
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
    this->send_command_async_(CMD_GET_VERSION, sizeof(CMD_GET_VERSION), 0);
    return;
  }

  // Read panel configuration once after model detection — one serial read per update
  // cycle. Config reads use blocking send_message_(), so they must NOT run in the same
  // cycle as async sensor/partition polling (otherwise both commands collide on the bus).
  //
  // Every multi-block step reads a single 64-byte block per cycle and only advances to
  // the next step when its read_*_() helper returns true, so no cycle blocks the main
  // loop on more than one transaction (zone/keyfob ESN reads are per-slot for the same
  // reason — 0xC0xx reads take ~1.5s each).
  if (this->config_read_step_ < 13 && this->communication_ok_) {
    switch (this->config_read_step_) {
      case 0: this->config_read_step_ = 1; break;  // skip one cycle after detection
      case 1: if (this->read_zone_config_()) this->config_read_step_ = 2; break;
      case 2: if (this->read_zone_names_()) this->config_read_step_ = 3; break;
      case 3:
        // Read one zone ESN per cycle; advance to step 4 when done
        if (this->read_zone_esn_next_())
          this->config_read_step_ = 4;
        break;
      case 4: if (this->read_output_names_()) this->config_read_step_ = 5; break;
      case 5: this->read_partition_config_(); this->config_read_step_ = 6; break;
      case 6: if (this->read_partition_names_()) this->config_read_step_ = 7; break;
      case 7: if (this->read_code_names_()) this->config_read_step_ = 8; break;
      case 8:
        // Read one keyfob ESN per cycle; advance to step 9 when done
        if (this->read_keyfob_esn_next_())
          this->config_read_step_ = 9;
        break;
      case 9: if (this->read_keyfob_names_()) this->config_read_step_ = 10; break;
      case 10: this->read_panel_mode_(); this->config_read_step_ = 11; break;
      case 11: this->read_status_flags_(); this->config_read_step_ = 12; break;
      case 12: this->publish_text_sensors_(); this->config_read_step_ = 13; break;
    }
    return;  // Skip normal polling this cycle — avoid bus collision
  }

  // On-demand event log dump (triggered by read_event_log button)
  if (this->event_log_read_pending_) {
    if (this->read_event_log_next_())
      this->event_log_read_pending_ = false;
    return;  // Skip normal polling this cycle
  }

  // On-demand debug memory scan (triggered by memory_scan button, issue #113)
  if (this->memory_scan_pending_) {
    if (this->memory_scan_next_())
      this->memory_scan_pending_ = false;
    return;  // Skip normal polling this cycle
  }

  // Re-publish text sensors periodically (every 120 polling cycles = ~60s at 500ms)
  // Text sensors are static config data but must be re-published so API clients
  // that connect after initial publish (e.g. Home Assistant reconnects) get the state.
  if (this->config_read_step_ >= 13) {
    this->text_sensor_republish_counter_++;
    if (this->force_publish_ || this->text_sensor_republish_counter_ >= 120) {
      this->read_panel_mode_();
      this->read_status_flags_();
      this->publish_text_sensors_();
      this->text_sensor_republish_counter_ = 0;
    }
  }

  // Normal polling: send sensor status query (partition query chains from loop())
  this->send_command_async_(CMD_GET_SENSOR_STATUS, sizeof(CMD_GET_SENSOR_STATUS), 1);

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
  // Some models (notably KYO32 non-G) don't support the version command and
  // return only the echo (6 bytes). In that case, we'll detect the model later
  // from the sensor status response length (see parse_sensor_status_).
  if (count <= 6) {
    ESP_LOGI(TAG, "Version query returned only %d bytes (echo only) — will detect model from sensor response", count);
    // Mark model as not detected but don't treat as error — let sensor status detection handle it
    this->model_detected_ = false;
    return true;  // Return true to avoid failure count, detection will happen via sensor response
  }

  if (count < RESP_VERSION) {
    ESP_LOGW(TAG, "Version query returned %d bytes (expected %d), attempting partial parse", count, RESP_VERSION);
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
  bool is_kyo8 = (this->alarm_model_ == AlarmModel::KYO_8 || this->alarm_model_ == AlarmModel::KYO_4 ||
                  this->alarm_model_ == AlarmModel::KYO_8G || this->alarm_model_ == AlarmModel::KYO_8W);

  // Determine parsing format from response length (12 = KYO8 format, 18 = KYO32 format).
  // Some panels report one model via firmware but use a different response format,
  // so always trust the response length for parsing.
  if (this->model_detected_) {
    int expected_len = is_kyo8 ? RESP_SENSOR_KYO8 : RESP_SENSOR_KYO32;
    if (count != expected_len) {
      if (count == RESP_SENSOR_KYO32 || count == RESP_SENSOR_KYO8) {
        is_kyo8 = (count == RESP_SENSOR_KYO8);
        ESP_LOGW(TAG, "Sensor status: %s firmware returned %d-byte response, parsing as %s format",
                 is_kyo8 ? "KYO32" : "KYO8", count, is_kyo8 ? "KYO8" : "KYO32");
      } else {
        ESP_LOGE(TAG, "Sensor status: expected %d bytes for %s model, got %d",
                 expected_len, is_kyo8 ? "KYO8" : "KYO32", count);
        return false;
      }
    }
  } else {
    // Model not yet detected — infer from response length
    switch (count) {
      case RESP_SENSOR_KYO32:
        this->alarm_model_ = AlarmModel::KYO_32;
        this->max_zones_ = KYO_MAX_ZONES;
        is_kyo8 = false;
        ESP_LOGI(TAG, "Detected KYO32 from sensor response length (18 bytes)");
        break;
      case RESP_SENSOR_KYO8:
        this->alarm_model_ = AlarmModel::KYO_8;
        this->max_zones_ = KYO_MAX_ZONES_8;
        is_kyo8 = true;
        ESP_LOGI(TAG, "Detected KYO8 from sensor response length (12 bytes)");
        break;
      default:
        ESP_LOGE(TAG, "Sensor status: invalid response length %d", count);
        return false;
    }
    this->model_detected_ = true;

    // Publish alarm model text sensor
    if (this->alarm_model_sensor_ != nullptr) {
      const char *model_name;
      switch (this->alarm_model_) {
        case AlarmModel::KYO_32: model_name = "KYO32"; break;
        case AlarmModel::KYO_8: model_name = "KYO8"; break;
        default: model_name = "Unknown"; break;
      }
      this->alarm_model_sensor_->publish_state(model_name);
    }
  }

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

bool BentelKyo::partition_status_looks_unmapped_(const uint8_t *rx, int count) const {
  if (count != RESP_PARTITION_KYO32)
    return false;
  // A configured partition is always in exactly one of armed_total/armed_partial/
  // armed_partial_delay0/disarmed, so a real response always has at least one bit set
  // across rx[6..9]. All-zero across all four means the register isn't mapped here.
  return (rx[6] | rx[7] | rx[8] | rx[9]) == 0;
}

bool BentelKyo::parse_partition_status_(const uint8_t *rx, int count) {
  bool is_kyo8 = (this->alarm_model_ == AlarmModel::KYO_8 || this->alarm_model_ == AlarmModel::KYO_4 ||
                  this->alarm_model_ == AlarmModel::KYO_8G);

  int expected_len = is_kyo8 ? RESP_PARTITION_KYO8 : RESP_PARTITION_KYO32;
  if (count != expected_len) {
    ESP_LOGE(TAG, "Partition status: expected %d bytes for %s model, got %d",
             expected_len, is_kyo8 ? "KYO8" : "KYO32", count);
    return false;
  }

  // Check cache
  bool changed = this->force_publish_ || (count != this->partition_cache_len_) ||
                 (memcmp(rx, this->partition_cache_, count) != 0);
  memcpy(this->partition_cache_, rx, count);
  this->partition_cache_len_ = count;

  if (!changed)
    return true;

  ESP_LOGD(TAG, "Partition status: total=0x%02X partial=0x%02X partial_d0=0x%02X disarmed=0x%02X rx10=0x%02X rx11=0x%02X rx12=0x%02X",
           rx[6], rx[7], rx[8], rx[9], rx[10], rx[11], rx[12]);

  // Parse partition arming states (same byte layout for both models)
  for (int i = 0; i < KYO_MAX_PARTITIONS; i++) {
    this->partition_armed_total_[i] = (rx[6] >> i) & 1;
    this->partition_armed_partial_[i] = (rx[7] >> i) & 1;
    this->partition_armed_partial_delay0_[i] = (rx[8] >> i) & 1;
    this->partition_disarmed_[i] = (rx[9] >> i) & 1;
  }

  // Model-dependent siren and output parsing
  if (is_kyo8) {
    if (this->alarm_model_ == AlarmModel::KYO_8W) {
      // KYO8W: rx[10] = siren byte, rx[12] = outputs 1-8
      // KYO8W was not in the old codebase; bit 6 is per contributor's addition
      this->siren_active_ = (rx[10] >> 6) & 1;
      for (int i = 0; i < 8; i++)
        this->output_state_[i] = (rx[12] >> i) & 1;
    } else {
      // KYO4/KYO8/KYO8G: rx[10] bit 5 = siren, bits 0-4 = outputs 1-5
      this->siren_active_ = (rx[10] >> 5) & 1;
      for (int i = 0; i < 5; i++)
        this->output_state_[i] = (rx[10] >> i) & 1;
    }
  } else {
    // KYO32/KYO32G: rx[10] bit 5 = siren, rx[11] = outputs 9-16, rx[12] = outputs 1-8
    this->siren_active_ = (rx[10] >> 5) & 1;
    for (int i = 0; i < 8; i++)
      this->output_state_[i] = (rx[12] >> i) & 1;
    for (int i = 0; i < 8; i++)
      this->output_state_[8 + i] = (rx[11] >> i) & 1;
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
      case BinarySensorType::PANEL_PROGRAMMING_MODE:
        state = this->panel_programming_mode_;
        break;
      case BinarySensorType::TROUBLE_ACTIVE:
        state = this->trouble_active_;
        break;
      case BinarySensorType::COMMUNICATION:
        // Handled separately in update()
        continue;
    }

    // Log state changes when log_trace is enabled
    if (this->log_trace_ && state != entry.sensor->state) {
      ESP_LOGI(TAG, "State change: %s [idx=%d] %s -> %s",
               entry.sensor->get_name().c_str(), idx,
               entry.sensor->state ? "ON" : "OFF",
               state ? "ON" : "OFF");
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
  uint8_t total_mask = 0x00, partial_mask = 0x00, partial_d0_mask = 0x00;
  for (int i = 0; i < KYO_MAX_PARTITIONS; i++) {
    if (this->partition_armed_total_[i]) total_mask |= (1 << i);
    if (this->partition_armed_partial_[i]) partial_mask |= (1 << i);
    if (this->partition_armed_partial_delay0_[i]) partial_d0_mask |= (1 << i);
  }

  uint8_t bit = 1 << (partition - 1);
  if (arm_type == 1)       // total (Away)
    total_mask |= bit;
  else if (arm_type == 2)  // partial (Stay/Home)
    partial_mask |= bit;
  else if (arm_type == 3)  // partial delay 0 (Night/Stay-0-Delay)
    partial_d0_mask |= bit;

  cmd[6] = total_mask;
  cmd[7] = partial_mask;
  cmd[8] = partial_d0_mask;
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

  // Read current arming state, clear this partition from all modes
  uint8_t total_mask = 0x00, partial_mask = 0x00, partial_d0_mask = 0x00;
  for (int i = 0; i < KYO_MAX_PARTITIONS; i++) {
    if (this->partition_armed_total_[i]) total_mask |= (1 << i);
    if (this->partition_armed_partial_[i]) partial_mask |= (1 << i);
    if (this->partition_armed_partial_delay0_[i]) partial_d0_mask |= (1 << i);
  }

  // Remove this partition from whichever mask it's in
  uint8_t clear = ~(1 << (partition - 1));
  total_mask &= clear;
  partial_mask &= clear;
  partial_d0_mask &= clear;

  cmd[6] = total_mask;
  cmd[7] = partial_mask;
  cmd[8] = partial_d0_mask;
  cmd[9] = calculate_crc_(cmd, 9);

  uint8_t rx[255];
  this->send_message_(cmd, sizeof(cmd), rx, 250);
}

void BentelKyo::arm_all_partitions(uint8_t arm_type) {
  ESP_LOGI(TAG, "Arm all partitions type %d", arm_type);
  uint8_t cmd[11] = {0x0F, 0x00, 0xF0, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0xCC, 0xFF};

  // Build masks from current state
  uint8_t total_mask = 0x00, partial_mask = 0x00, partial_d0_mask = 0x00;
  for (int i = 0; i < KYO_MAX_PARTITIONS; i++) {
    if (this->partition_armed_total_[i]) total_mask |= (1 << i);
    if (this->partition_armed_partial_[i]) partial_mask |= (1 << i);
    if (this->partition_armed_partial_delay0_[i]) partial_d0_mask |= (1 << i);
  }

  // Set all registered partition bits
  for (auto *panel : this->alarm_panels_) {
    uint8_t bit = 1 << (panel->get_partition() - 1);
    if (arm_type == 1)
      total_mask |= bit;
    else if (arm_type == 2)
      partial_mask |= bit;
    else if (arm_type == 3)
      partial_d0_mask |= bit;
  }

  cmd[6] = total_mask;
  cmd[7] = partial_mask;
  cmd[8] = partial_d0_mask;
  cmd[9] = calculate_crc_(cmd, 9);

  uint8_t rx[255];
  this->send_message_(cmd, sizeof(cmd), rx, 250);
}

void BentelKyo::disarm_all_partitions() {
  ESP_LOGI(TAG, "Disarm all partitions");
  // Send all-zero masks unconditionally — same as upstream specific_area=0
  uint8_t cmd[11] = {0x0F, 0x00, 0xF0, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0xFF, 0xFF};
  cmd[9] = calculate_crc_(cmd, 9);

  uint8_t rx[255];
  this->send_message_(cmd, sizeof(cmd), rx, 250);
}

void BentelKyo::arm_preset(uint8_t total_mask, uint8_t partial_mask,
                           uint8_t partial_d0_mask) {
  ESP_LOGI(TAG, "Arm preset: total=0x%02X partial=0x%02X partial_d0=0x%02X",
           total_mask, partial_mask, partial_d0_mask);
  uint8_t cmd[11] = {0x0F, 0x00, 0xF0, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0xCC, 0xFF};

  // Send preset masks directly — unconfigured partitions get 0 (disarmed)
  // This matches upstream specific_area=0 behavior: only the specified
  // partition bits are set, everything else goes to zero.
  cmd[6] = total_mask;
  cmd[7] = partial_mask;
  cmd[8] = partial_d0_mask;
  cmd[9] = calculate_crc_(cmd, 9);

  uint8_t rx[255];
  this->send_message_(cmd, sizeof(cmd), rx, 250);
}

void BentelKyo::reset_alarms() {
  ESP_LOGI(TAG, "Reset alarms");
  uint8_t rx[255];
  this->send_message_(CMD_RESET_ALARMS, sizeof(CMD_RESET_ALARMS), rx, 250);
}

void BentelKyo::activate_output(uint8_t output_number) {
  // Command uses single-byte bitmask — only outputs 1-8 fit
  if (output_number < 1 || output_number > 8) {
    ESP_LOGE(TAG, "Invalid output %d (1-8)", output_number);
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
  // Command uses single-byte bitmask — only outputs 1-8 fit
  if (output_number < 1 || output_number > 8) {
    ESP_LOGE(TAG, "Invalid output %d (1-8)", output_number);
    return;
  }

  ESP_LOGI(TAG, "Deactivate output %d", output_number);
  uint8_t cmd[9] = {0x0F, 0x06, 0xF0, 0x01, 0x00, 0x06, 0x00, 0x00, 0xCC};
  cmd[7] = 1 << (output_number - 1);
  cmd[8] = cmd[7];

  uint8_t rx[255];
  this->send_message_(cmd, sizeof(cmd), rx, 250);
}

void BentelKyo::pulse_output(uint8_t output_number, uint32_t pulse_time_ms) {
  if (output_number < 1 || output_number > 8) {
    ESP_LOGE(TAG, "Invalid output %d (1-8)", output_number);
    return;
  }
  if (pulse_time_ms < 50 || pulse_time_ms > 30000) {
    ESP_LOGE(TAG, "Invalid pulse time %lu ms (50-30000)", (unsigned long) pulse_time_ms);
    return;
  }

  ESP_LOGI(TAG, "Pulse output %d for %lu ms", output_number, (unsigned long) pulse_time_ms);
  this->activate_output(output_number);
  uint32_t end_ms = millis() + pulse_time_ms;
  this->pulse_output_end_ms_[output_number - 1] = end_ms == 0 ? 1 : end_ms;
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

void BentelKyo::sync_datetime_from_ntp() {
  // Get current local time from the system clock (synced by SNTP if configured)
  auto now = ESPTime::from_epoch_local(::time(nullptr));
  if (!now.is_valid()) {
    ESP_LOGW(TAG, "NTP time not available yet, cannot sync datetime");
    return;
  }

  ESP_LOGI(TAG, "Syncing panel datetime from NTP: %02d/%02d/%04d %02d:%02d:%02d",
           now.day_of_month, now.month, now.year, now.hour, now.minute, now.second);
  this->update_datetime(now.day_of_month, now.month, now.year,
                        now.hour, now.minute, now.second);
}

// ========================================
// Serial I/O
// ========================================

int BentelKyo::send_message_(const uint8_t *cmd, int cmd_len, uint8_t *response, uint32_t timeout_ms) {
  // Cancel any in-flight async transaction so loop() won't steal our bytes
  bool aborted_async = false;
  if (this->serial_state_ == SerialState::WAITING_RESPONSE) {
    ESP_LOGD(TAG, "Aborting in-flight async poll for blocking command");
    this->serial_state_ = SerialState::IDLE;
    aborted_async = true;
  }

  // Wait for bus silence — drain any remaining panel response bytes.
  // After aborting an async poll, the panel may still be processing the
  // previous query (variable 10-50ms gap before it starts responding).
  // Use a longer silence window to ensure the panel has finished.
  uint32_t silence_ms = aborted_async ? 100 : 20;
  uint32_t quiet_start = millis();
  while ((millis() - quiet_start) < silence_ms) {
    if (this->available() > 0) {
      while (this->available() > 0)
        this->read();
      quiet_start = millis();  // Reset silence timer
    }
    yield();
  }

  // Send command
  if (this->serial_trace_) {
    ESP_LOGI(TAG, "TX [%d bytes]: %s", cmd_len,
             format_hex_pretty(cmd, cmd_len).c_str());
  }
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

  if (this->serial_trace_) {
    ESP_LOGI(TAG, "RX [%d bytes]: %s", index,
             format_hex_pretty(rx_buf, index).c_str());
  }

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

bool BentelKyo::read_zone_config_() {
  // Zone config: 4 bytes per zone, 16 zones per 64-byte read.
  // One read per update cycle (block 0 = zones 1-16, block 1 = 17-32); returns true
  // when done so the loop is never blocked for long.
  //
  // KYO32G 2.13 places the table six bytes later than the non-G map documented in
  // PROTOCOL.md 10.1 — records start at 0x00A5/0x00E5, not 0x009F/0x00DF. Reading the
  // shifted base (rather than adding a header offset like the KYO8 path below) keeps
  // all 16 records inside the 64 returned bytes.
  static const uint16_t BASE_ADDRS_NONG[] = {0x009F, 0x00DF};
  static const uint16_t BASE_ADDRS_KYO32G[] = {0x00A5, 0x00E5};
  const uint16_t *base_addrs =
      (this->alarm_model_ == AlarmModel::KYO_32G) ? BASE_ADDRS_KYO32G : BASE_ADDRS_NONG;
  int num_blocks = (this->max_zones_ > 16) ? 2 : 1;
  int blk = this->config_chunk_index_;

  uint8_t rx[255];
  int count = this->read_register_(base_addrs[blk], 0x3F, rx, 300);
  if (count < 6 + 64) {
    ESP_LOGW(TAG, "Zone config read at 0x%04X failed: got %d bytes", base_addrs[blk], count);
  } else {
    // KYO8 2.04 prefixes the block with a 4-byte header, so zone records start at 0x00A3
    // instead of 0x009F (issue #113 validation: with the shift, every zone lands in the
    // area configured on the panel; without it zone 1 reads area 0x00 = "None").
    // KYO8 record bytes, confirmed by Suite-vs-scan differentials on firmware 2.04:
    // [0]=type in the low bits (0x00 Instant, 0x01 Delayed, 0x02 Path) plus wiring/balance
    // in the high bits (Normally Closed = 0x00, Normally Open = +0x08), [1]=zone attribute
    // bitmask (Internal = 0x20; 0x00 = no attributes set — NOT a wireless-enrolled flag),
    // [2]=area mask, [3]=alarm-cycle count (0-14; 0x0F = repetitive/unlimited).
    // Note the layout differs from KYO32 (section 10.1 of PROTOCOL.md), where [1] is the
    // enrolled flag and [3] is the attribute byte.
    // The header also means only 15 full records fit in the 64 returned bytes — cap the
    // loop so a hypothetical >8-zone KYO8 read can never index past the buffer.
    bool kyo8 = this->is_kyo8_family_();
    int base = 6 + (kyo8 ? 4 : 0);
    int max_recs = kyo8 ? 8 : 16;
    for (int i = 0; i < max_recs; i++) {
      int z = blk * 16 + i;
      if (z >= this->max_zones_)
        break;
      int offset = base + (i * 4);
      this->zone_type_raw_[z] = rx[offset];
      this->zone_enrolled_[z] = (rx[offset + 1] == 0x01);
      this->zone_area_mask_[z] = rx[offset + 2];
      ESP_LOGD(TAG, "Zone %d: type=0x%02X, area=0x%02X, enrolled=%d", z + 1,
               this->zone_type_raw_[z], this->zone_area_mask_[z], this->zone_enrolled_[z]);
    }
  }

  if (++this->config_chunk_index_ >= num_blocks) {
    this->config_chunk_index_ = 0;
    return true;
  }
  return false;
}

bool BentelKyo::is_kyo8_family_() const {
  // KYO8W is intentionally excluded. Despite the "KYO8W"/"KYO8WG" firmware name it uses
  // KYO32-format status/partition responses (issue #107, fixed by PR #109), and the runtime
  // paths already route it through KYO32 (see the is_kyo8 checks at the partition command
  // selection and parser). We have no KYO8W config-memory trace, so its name tables and fixed
  // registers stay on the KYO32 (non-G) map — the pre-#113 behavior — rather than the KYO8
  // 0x3250 map, until a KYO8W trace confirms its layout.
  return this->alarm_model_ == AlarmModel::KYO_4 || this->alarm_model_ == AlarmModel::KYO_8 ||
         this->alarm_model_ == AlarmModel::KYO_8G;
}

const uint16_t *BentelKyo::select_name_bases_(const uint16_t *nong, const uint16_t *kyo32g,
                                              const uint16_t *kyo8) const {
  if (this->alarm_model_ == AlarmModel::KYO_32G)
    return kyo32g;
  if (this->is_kyo8_family_())
    return kyo8;
  return nong;
}

// Reads one 64-byte block (up to 4 names) of a name table per call, advancing
// config_chunk_index_. Returns true when the whole table has been read. Keeps each
// update() cycle short (one serial transaction) instead of blocking on several.
bool BentelKyo::read_name_table_chunk_(const uint16_t *bases, int num_blocks, int count,
                                       std::string *out, const char *what) {
  if (bases == nullptr) {
    // Address map not yet known for this model (issue #113) — skip rather than decode
    // garbage from a wrong address. Names stay empty.
    ESP_LOGW(TAG,
             "%s names: address map unknown for firmware '%s' — skipped. Please open an issue at "
             "https://github.com/lorenzo-deluca/espkyogate with a 'serial_trace' dump to map it.",
             what, this->firmware_version_);
    this->config_chunk_index_ = 0;
    return true;
  }
  int blk = this->config_chunk_index_;
  uint8_t rx[255];
  int rc = this->read_register_(bases[blk], 0x3F, rx, 300);
  if (rc < 6 + 64) {
    ESP_LOGW(TAG, "%s names read at 0x%04X failed: got %d bytes", what, bases[blk], rc);
  } else {
    for (int n = 0; n < 4; n++) {
      int idx = blk * 4 + n;
      if (idx >= count)
        break;
      out[idx] = decode_panel_name(&rx[6 + n * 16], 16);
      ESP_LOGD(TAG, "%s %d name: '%s'", what, idx + 1, out[idx].c_str());
    }
  }

  if (++this->config_chunk_index_ >= num_blocks) {
    this->config_chunk_index_ = 0;
    return true;
  }
  return false;
}

bool BentelKyo::read_zone_names_() {
  // Zone names: 16 ASCII bytes per zone, 4 zones per 64-byte read.
  // KYO32G keeps the user-programmed labels lower than the non-G map (issue #93):
  // zone names at 0x19B0-0x1BAF instead of 0x2E00-0x2FFF.
  static const uint16_t BASE_ADDRS_NONG[] = {0x2E00, 0x2E40, 0x2E80, 0x2EC0, 0x2F00, 0x2F40, 0x2F80, 0x2FC0};
  static const uint16_t BASE_ADDRS_32G[] = {0x19B0, 0x19F0, 0x1A30, 0x1A70, 0x1AB0, 0x1AF0, 0x1B30, 0x1B70};
  // KYO8 2.04 keeps the user-label table higher, starting at 0x3250 (issue #113):
  // 8 zone names in two 64-byte blocks (0x3250-0x328F, 0x3290-0x32CF). Confirmed against
  // the 0x009F zone-config block.
  static const uint16_t BASE_ADDRS_KYO8[] = {0x3250, 0x3290};
  const uint16_t *bases = this->select_name_bases_(BASE_ADDRS_NONG, BASE_ADDRS_32G, BASE_ADDRS_KYO8);
  int num_blocks = (this->max_zones_ <= 8) ? 2 : 8;
  return this->read_name_table_chunk_(bases, num_blocks, this->max_zones_, this->zone_name_, "Zone");
}

bool BentelKyo::read_zone_esn_next_() {
  // Zone ESN at 0xC045: 3 bytes per zone, per-zone reads with stride 3
  // Reads ONE zone per call (one per update cycle) to avoid blocking the main loop.
  // USB capture shows panel takes ~1s to respond to 0xC0xx reads (EEPROM access).
  // Returns true when all zones have been read.
  if (this->is_kyo8_family_()) {
    // The 0xC045/0xC0B1 ESN windows read unrelated config data on KYO8 2.04 — a panel
    // with no wireless receiver returned non-empty "serials", with area-timer bytes
    // (0F/1E/14) visible in the higher slots (issue #113 validation). Skip; the ESN
    // sensors stay at "N/A".
    this->esn_read_index_ = 0;
    return true;
  }
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

bool BentelKyo::read_output_names_() {
  // Output names: 16 ASCII bytes per output, 4 per 64-byte read, 16 outputs total.
  // KYO32G stores the programmable label table at a different base than KYO32/KYO32M:
  // the documented 0x3280-0x337F range holds a built-in (English) default label ROM,
  // while the user-programmed labels live lower. Outputs were located at 0x1E30-0x1F2F
  // via an on-device memory scan (issue #93).
  static const uint16_t BASE_ADDRS_NONG[] = {0x3280, 0x32C0, 0x3300, 0x3340};
  static const uint16_t BASE_ADDRS_32G[] = {0x1E30, 0x1E70, 0x1EB0, 0x1EF0};
  // KYO8 2.04: output names live at 0x3710, after the activator labels 0x3610-0x370F, in
  // the contiguous user-label table (issue #113 memory scan). KYO4/8 expose 5 outputs
  // (status bits 0-4), so two 64-byte blocks cover slots 1-5; 0x3760+ holds phone-number
  // labels, which is why only 5 slots are read.
  static const uint16_t BASE_ADDRS_KYO8[] = {0x3710, 0x3750};
  const uint16_t *bases = this->select_name_bases_(BASE_ADDRS_NONG, BASE_ADDRS_32G, BASE_ADDRS_KYO8);
  bool kyo8 = this->is_kyo8_family_();
  int num_blocks = kyo8 ? 2 : 4;
  int count = kyo8 ? KYO_OUTPUTS_8 : KYO_MAX_OUTPUTS;
  return this->read_name_table_chunk_(bases, num_blocks, count, this->output_name_, "Output");
}

void BentelKyo::read_partition_config_() {
  // Timers at 0x016F: 26 bytes total (section 10.5)
  // Bytes 0-15: entry/exit timers (2 bytes per partition: entry, exit) for 8 partitions
  // Bytes 16-23: siren duration (1 byte per partition)
  if (this->is_kyo8_family_()) {
    // KYO8 2.04 keeps the area timers right after the zone-config block, not at 0x016F
    // (which returns an index table on this firmware). Layout pinned by differential
    // captures and cross-checked field by field against the Bentel Security Suite
    // "Areas" page (issue #113; see PROTOCOL.md 9.2 for the original Suite UI names):
    //   0x00DC-0x00DF: exit delay P1-P4 (seconds)
    //   0x00E0-0x00E3: entry delay P1-P4 (seconds)
    //   0x00E4-0x00E7: scheduler advance-warning time P1-P4 (minutes)
    //   0x00E8-0x00EB: And-Zone time P1-P4 (15-second steps)
    //   0x00EC-0x00EF: And-Code time P1-P4 (seconds)
    //   0x00F8:        patrol time, global (minutes)
    //   0x00FA:        alarm-cycle/siren duration, global (minutes, 0-63;
    //                  0 = siren outputs never fire)
    uint8_t rx[255];
    int count = this->read_register_(0x00DC, 0x1F, rx, 300);
    if (count < 6 + 31) {
      ESP_LOGW(TAG, "KYO8 timer read at 0x00DC failed: got %d bytes", count);
      return;
    }
    uint8_t siren_min = rx[6 + 0x1E];  // 0x00FA
    for (int i = 0; i < KYO_PARTITIONS_8; i++) {
      this->partition_exit_delay_[i] = rx[6 + i];
      this->partition_entry_delay_[i] = rx[6 + 4 + i];
      this->partition_siren_timer_[i] = siren_min;
      ESP_LOGD(TAG, "Partition %d: entry=%ds, exit=%ds, siren=%dmin", i + 1,
               this->partition_entry_delay_[i], this->partition_exit_delay_[i], siren_min);
    }
    return;
  }
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
  if (this->is_kyo8_family_()) {
    // Same as zone ESNs: the 0xC0B1 window reads unrelated config data on KYO8 2.04
    // (issue #113 validation — all 16 slots "populated" on a panel with zero keyfobs).
    // Skip; the keyfob ESN sensors stay at "N/A".
    this->keyfob_read_index_ = 0;
    return true;
  }
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

bool BentelKyo::read_keyfob_names_() {
  // Keyfob names: 16 ASCII bytes per keyfob, 4 per 64-byte read, 16 keyfobs total.
  // KYO32G stores them lower than the non-G map (issue #93): keyfob names
  // at 0x1D30-0x1E2F instead of 0x3180-0x327F.
  static const uint16_t BASE_ADDRS_NONG[] = {0x3180, 0x31C0, 0x3200, 0x3240};
  static const uint16_t BASE_ADDRS_32G[] = {0x1D30, 0x1D70, 0x1DB0, 0x1DF0};
  // KYO8 2.04: keyfob (digital key) names live at 0x3610, labelled "Attivatore 1-16" by
  // default (issue #113 memory scan). The table sits between the code and output names
  // with 16 slots — the same relative position and count as the non-G "digital key names"
  // table (see PROTOCOL.md section 9.1) — "attivatore" being Bentel's term for the digital
  // keys presented to readers.
  static const uint16_t BASE_ADDRS_KYO8[] = {0x3610, 0x3650, 0x3690, 0x36D0};
  const uint16_t *bases = this->select_name_bases_(BASE_ADDRS_NONG, BASE_ADDRS_32G, BASE_ADDRS_KYO8);
  return this->read_name_table_chunk_(bases, 4, KYO_MAX_KEYFOBS, this->keyfob_name_, "Keyfob");
}

bool BentelKyo::read_partition_names_() {
  // Partition names: 16 ASCII bytes per partition, 4 per 64-byte read, 8 partitions total.
  // KYO32G stores them lower than the non-G map (issue #93): partition names
  // at 0x1750-0x17CF instead of 0x2BA0-0x2C1F.
  static const uint16_t BASE_ADDRS_NONG[] = {0x2BA0, 0x2BE0};
  static const uint16_t BASE_ADDRS_32G[] = {0x1750, 0x1790};
  // KYO8 2.04: the area names sit at 0x32D0, right after the 8 zone names (issue #113,
  // confirmed against the panel keypad). KYO4/8 have 4 partitions (see the ranges_kyo8
  // table in decode_event_code_), so a single 64-byte block covers them; 0x3310+ holds
  // keypad labels, which is why only 4 slots are read.
  static const uint16_t BASE_ADDRS_KYO8[] = {0x32D0};
  const uint16_t *bases = this->select_name_bases_(BASE_ADDRS_NONG, BASE_ADDRS_32G, BASE_ADDRS_KYO8);
  bool kyo8 = this->is_kyo8_family_();
  int num_blocks = kyo8 ? 1 : 2;
  int count = kyo8 ? KYO_PARTITIONS_8 : KYO_MAX_PARTITIONS;
  return this->read_name_table_chunk_(bases, num_blocks, count, this->partition_name_, "Partition");
}

bool BentelKyo::read_code_names_() {
  // Code names: 16 ASCII bytes per code, 4 per 64-byte read, 24 codes total.
  // KYO32G stores them lower than the non-G map (issue #93): code names
  // at 0x1BB0-0x1D2F instead of 0x3000-0x317F.
  static const uint16_t BASE_ADDRS_NONG[] = {0x3000, 0x3040, 0x3080, 0x30C0, 0x3100, 0x3140};
  static const uint16_t BASE_ADDRS_32G[] = {0x1BB0, 0x1BF0, 0x1C30, 0x1C70, 0x1CB0, 0x1CF0};
  // KYO8 2.04: code names live at 0x3490, after the reader labels 0x3390-0x348F, in the
  // contiguous user-label table (located via the issue #113 on-device memory scan; slot 1
  // matched the panel's configured code name). 24 slots of 16 bytes, 6 blocks of 4.
  static const uint16_t BASE_ADDRS_KYO8[] = {0x3490, 0x34D0, 0x3510, 0x3550, 0x3590, 0x35D0};
  const uint16_t *bases = this->select_name_bases_(BASE_ADDRS_NONG, BASE_ADDRS_32G, BASE_ADDRS_KYO8);
  return this->read_name_table_chunk_(bases, 6, KYO_MAX_CODES, this->code_name_, "Code");
}

void BentelKyo::read_panel_mode_() {
  if (this->is_kyo8_family_()) {
    // 0x01E6 is not the panel-mode register on KYO8 2.04 — it returns 00 00, which the
    // {0x11,0x10} idle baseline reads as a permanent false programming=YES (issue #113).
    // Leave panel_programming_mode_ at its idle default; the real state, if needed, is in
    // the continuous F0 68 status poll.
    return;
  }
  if (this->alarm_model_ == AlarmModel::KYO_32G) {
    // 0x01E6 is not the panel-mode register on KYO32G. A full config scan of a KYO32G 2.13
    // shows the 32-zone config table filling 0x009F-0x011E and the zone-enrollment table at
    // 0x019E-0x01F7, so 0x01E6 lands inside enrollment (reads FF 00 = the 2nd byte of a zone
    // record). Against the {0x11,0x10} idle baseline that is a permanent false programming=YES.
    // Leave panel_programming_mode_ at its idle default. A programming-mode differential can't
    // locate the real register either: on KYO32G (as on KYO8) entering the installer menu
    // silences the serial bus entirely, so the communication-status sensor is the only
    // observable signal for that state.
    return;
  }
  uint8_t rx[255];
  int count = this->read_register_(0x01E6, 0x02, rx, 300);
  if (count < 6 + 2) {
    ESP_LOGW(TAG, "Panel mode read failed: got %d bytes", count);
    return;
  }

  this->panel_mode_raw_[0] = rx[6];
  this->panel_mode_raw_[1] = rx[7];

  // Programming mode = bytes differ from idle baseline {0x11, 0x10}
  this->panel_programming_mode_ = (rx[6] != 0x11 || rx[7] != 0x10);

  ESP_LOGD(TAG, "Panel mode: %02X %02X (programming=%s)",
           rx[6], rx[7], this->panel_programming_mode_ ? "YES" : "no");
}

void BentelKyo::read_status_flags_() {
  if (this->is_kyo8_family_()) {
    // 0x1503 reads ASCII text, not trouble bit-flags, on KYO8 2.04, which the "any byte
    // != 0xFF" rule reads as a permanent false trouble=YES (issue #113). Leave
    // trouble_active_ at its no-trouble default until the real register is located.
    return;
  }
  if (this->alarm_model_ == AlarmModel::KYO_32G) {
    // 0x1503 is not the trouble-flags register on KYO32G. The 2.13 scan shows this address
    // sits in the FF padding just after the partition-status block (0x14EC/0x1502 on the G),
    // reading 00 00 FF 00 FF; the "any byte != 0xFF" rule turns that into a permanent false
    // trouble=YES. Leave trouble_active_ at its no-trouble default. Real faults still surface
    // through the WARNING_* binary sensors parsed from the live status poll.
    return;
  }
  uint8_t rx[255];
  int count = this->read_register_(0x1503, 0x05, rx, 300);
  if (count < 6 + 5) {
    ESP_LOGW(TAG, "Status flags read failed: got %d bytes", count);
    return;
  }

  for (int i = 0; i < 5; i++)
    this->status_flags_raw_[i] = rx[6 + i];

  // Trouble active = any byte != 0xFF (all-FF = no troubles)
  this->trouble_active_ = false;
  for (int i = 0; i < 5; i++) {
    if (rx[6 + i] != 0xFF) {
      this->trouble_active_ = true;
      break;
    }
  }

  ESP_LOGD(TAG, "Status flags: %02X %02X %02X %02X %02X (trouble=%s)",
           rx[6], rx[7], rx[8], rx[9], rx[10],
           this->trouble_active_ ? "YES" : "no");
}

const char *BentelKyo::decode_event_code_(uint16_t code, uint8_t *entity_out, char *buf, size_t buf_len) {
  // Event code table from BIS KYO Unit PDF + KyoUnit 5.5 serial capture correlation.
  //
  // The 16-bit event code encodes both event type and entity number:
  //   code = base + (entity_number - 1)
  // where entity_number is 1-based (partition 1-8, zone 1-32, code 1-24, key 1-128).
  //
  // Entity counts differ between KYO32 and KYO4/8 — table is selected by alarm_model_.

  struct EventRange {
    uint16_t base;
    uint8_t count;  // max entities (0 = no entity)
    const char *name;
    const char *entity_type;  // "partition", "zone", "code", "key", or nullptr
  };

  bool is_kyo8 = (this->alarm_model_ == AlarmModel::KYO_4 ||
                  this->alarm_model_ == AlarmModel::KYO_8 ||
                  this->alarm_model_ == AlarmModel::KYO_8G ||
                  this->alarm_model_ == AlarmModel::KYO_8W);

  // KYO32 table: 8 partitions, 32 zones, 24 codes, 128 keys — sorted by base offset
  static const EventRange ranges_kyo32[] = {
    {0x0000, 8,   "Alarm Partition",              "partition"},
    {0x0008, 32,  "Alarm Zone",                   "zone"},
    {0x0028, 8,   "Inactivity Area",              "partition"},
    {0x0030, 8,   "Negligence Area",              "partition"},
    {0x0038, 32,  "Zone Bypass",                  "zone"},
    {0x0058, 32,  "Zone Unbypass",                "zone"},
    {0x0078, 24,  "Recognized Code",              "code"},
    {0x0090, 128, "Recognized Key",               "key"},
    {0x0110, 32,  "Auto Bypass Zone",             "zone"},
    {0x0130, 8,   "Arm Partition",                "partition"},
    {0x0138, 8,   "Disarm Partition",             "partition"},
    {0x0140, 8,   "Special Arming Partition",     "partition"},
    {0x0148, 8,   "Special Disarming Partition",  "partition"},
    {0x0150, 8,   "Reset Memory Partition",       "partition"},
    {0x0158, 8,   "Coercion Disarm Area",         "partition"},
    {0x0160, 0,   "Failed Call",                  nullptr},
    {0x0168, 32,  "Tamper Zone",                  "zone"},
    {0x0188, 32,  "Restore Zone",                 "zone"},
    {0x01BC, 0,   "Remote Command",               nullptr},
  };

  // KYO4/8 table: 4 partitions, 8 zones, 8 codes, 16 keys — sorted by base offset
  static const EventRange ranges_kyo8[] = {
    {0x0000, 4,   "Alarm Partition",              "partition"},
    {0x0004, 8,   "Alarm Zone",                   "zone"},
    {0x000C, 4,   "Inactivity Area",              "partition"},
    {0x0010, 4,   "Negligence Area",              "partition"},
    {0x0014, 8,   "Zone Bypass",                  "zone"},
    {0x001C, 8,   "Zone Unbypass",                "zone"},
    {0x0024, 8,   "Recognized Code",              "code"},
    {0x002C, 16,  "Recognized Key",               "key"},
    {0x003C, 8,   "Auto Bypass Zone",             "zone"},
    {0x0044, 4,   "Arm Partition",                "partition"},
    {0x0048, 4,   "Disarm Partition",             "partition"},
    {0x004C, 4,   "Special Arming Partition",     "partition"},
    {0x0050, 4,   "Special Disarming Partition",  "partition"},
    {0x0054, 4,   "Reset Memory Partition",       "partition"},
    {0x0058, 4,   "Coercion Disarm Area",         "partition"},
    {0x005C, 0,   "Failed Call",                  nullptr},
    {0x0060, 8,   "Tamper Zone",                  "zone"},
    {0x0068, 8,   "Restore Zone",                 "zone"},
    {0x0070, 0,   "Remote Command",               nullptr},
  };

  const EventRange *ranges;
  int num_ranges;
  if (is_kyo8) {
    ranges = ranges_kyo8;
    num_ranges = sizeof(ranges_kyo8) / sizeof(ranges_kyo8[0]);
  } else {
    ranges = ranges_kyo32;
    num_ranges = sizeof(ranges_kyo32) / sizeof(ranges_kyo32[0]);
  }

  for (int i = 0; i < num_ranges; i++) {
    const auto &range = ranges[i];
    if (range.count == 0) {
      if (code == range.base) {
        *entity_out = 0;
        return range.name;
      }
    } else {
      uint16_t offset = code - range.base;
      if (code >= range.base && offset < range.count) {
        *entity_out = offset + 1;  // 1-based
        return range.name;
      }
    }
  }

  // Unknown event code — format as hex
  *entity_out = 0;
  snprintf(buf, buf_len, "Unknown (0x%04X)", code);
  return buf;
}

bool BentelKyo::read_event_log_next_() {
  static const int EVENT_LOG_CHUNKS = 28;
  static const uint16_t EVENT_LOG_BASE = 0x0D27;
  static const int RECORDS_PER_CHUNK = 9;  // 63 bytes / 7 bytes per record

  int chunk = this->event_log_chunk_index_;
  if (chunk >= EVENT_LOG_CHUNKS) {
    ESP_LOGI(TAG, "Event log dump complete (%d entries logged)", this->event_log_entries_logged_);
    return true;
  }

  uint16_t addr = EVENT_LOG_BASE + (chunk * 0x40);
  ESP_LOGD(TAG, "Event log chunk %d/28 (0x%04X)", chunk + 1, addr);

  uint8_t rx[255];
  int count = this->read_register_(addr, 0x3F, rx, 500);
  if (count < 6 + 63) {
    ESP_LOGW(TAG, "Event log chunk %d read failed at 0x%04X: got %d bytes", chunk + 1, addr, count);
    this->event_log_chunk_index_++;
    return false;
  }

  for (int i = 0; i < RECORDS_PER_CHUNK; i++) {
    int slot = chunk * RECORDS_PER_CHUNK + i;
    int offset = 6 + (i * 7);

    uint8_t code_hi = rx[offset];
    uint8_t code_lo = rx[offset + 1];
    uint8_t day = rx[offset + 2];
    uint8_t month = rx[offset + 3];
    uint8_t year = rx[offset + 4];
    uint8_t hour = rx[offset + 5];
    uint8_t minute = rx[offset + 6];

    // Skip empty records (all zeros)
    if (code_hi == 0x00 && code_lo == 0x00 && day == 0x00 && month == 0x00)
      continue;

    uint16_t event_code = (code_hi << 8) | code_lo;
    uint8_t entity = 0;
    char unknown_buf[24];
    const char *event_name = decode_event_code_(event_code, &entity, unknown_buf, sizeof(unknown_buf));

    if (entity > 0) {
      ESP_LOGI(TAG, "Event [%03d]: %02d-%02d-%04d %02d:%02d  %s n.%d",
               slot + 1, day, month, 2000 + year, hour, minute, event_name, entity);
    } else {
      ESP_LOGI(TAG, "Event [%03d]: %02d-%02d-%04d %02d:%02d  %s",
               slot + 1, day, month, 2000 + year, hour, minute, event_name);
    }
    this->event_log_entries_logged_++;
  }

  this->event_log_chunk_index_++;
  return false;
}

// Debug helper for issue #113: dumps memory regions whose KYO8 layout is still unknown,
// one 64-byte read per update() cycle, as hex + printable ASCII rows (16 bytes per row,
// matching the 16-byte name-slot size so label tables line up visually).
//
// Regions scanned (v5 — full sweep of the documented config space, for cross-model
// comparison; the goal is a complete map on every model, not just KYO8):
// - 0x0000-0x09FF: core configuration. Covers everything in PROTOCOL.md section 10 up to
//   the end of event routing (0x0921) — zone config, keyfob buttons, timers, enrollment,
//   partition/code config, panel options, ARC phone numbers — plus the undocumented
//   0x0000-0x009E head. On KYO8 this includes the whole 9.2 timer/telephony block.
// - 0x14C0-0x153F: runtime status region (partition status 0x14EC/0x1502, status flags
//   0x1503-0x1509). Bytes here change with panel state, so diffs are only meaningful for
//   config-differential captures if the panel state is otherwise unchanged.
// - 0x2BA0-0x381F: all name tables on both map families (KYO32 non-G: 0x2BA0-0x347F,
//   sections 10.2/10.6-10.13; KYO8 2.04: 0x3250-0x37EF, section 9.2).
// Deliberately excluded: the event log (0x0D27-0x1426, dumpable via the read_event_log
// button) and the slow 0xC0xx EEPROM windows (~1.5s per read, already handled by the
// ESN readers). Reads of addresses a model doesn't answer for fail after the timeout and
// are logged and skipped — the scan always runs to completion.
bool BentelKyo::memory_scan_next_() {
  struct ScanRange {
    uint16_t start;
    uint8_t chunks;  // number of 64-byte reads
  };
  static const ScanRange SCAN_RANGES[] = {
      {0x0000, 40},  // 0x0000-0x09FF core config
      {0x14C0, 2},   // 0x14C0-0x153F status region
      {0x2BA0, 50},  // 0x2BA0-0x381F name tables
  };
  static const int SCAN_RANGE_COUNT = sizeof(SCAN_RANGES) / sizeof(SCAN_RANGES[0]);

  int total_chunks = 0;
  for (int r = 0; r < SCAN_RANGE_COUNT; r++)
    total_chunks += SCAN_RANGES[r].chunks;

  int chunk = this->memory_scan_chunk_index_;
  if (chunk >= total_chunks) {
    ESP_LOGI(TAG, "Memory scan complete — redact personal data, then attach the SCAN lines above to issue #113");
    return true;
  }

  uint16_t addr = 0;
  int rem = chunk;
  for (int r = 0; r < SCAN_RANGE_COUNT; r++) {
    if (rem < SCAN_RANGES[r].chunks) {
      addr = SCAN_RANGES[r].start + (uint16_t) (rem * 64);
      break;
    }
    rem -= SCAN_RANGES[r].chunks;
  }
  ESP_LOGI(TAG, "Memory scan chunk %d/%d (0x%04X)", chunk + 1, total_chunks, addr);

  uint8_t rx[255];
  int count = this->read_register_(addr, 0x3F, rx, 500);
  if (count < 6 + 64) {
    ESP_LOGW(TAG, "SCAN 0x%04X: read failed (%d bytes) — register may not exist on this panel", addr, count);
    this->memory_scan_chunk_index_++;
    return false;
  }

  // 4 rows of 16 bytes: hex dump + printable ASCII (non-printable -> '.')
  for (int row = 0; row < 4; row++) {
    const uint8_t *p = &rx[6 + row * 16];
    char hex[16 * 3 + 1];
    char ascii[16 + 1];
    for (int i = 0; i < 16; i++) {
      snprintf(&hex[i * 3], 4, "%02X ", p[i]);
      ascii[i] = (p[i] >= 0x20 && p[i] <= 0x7E) ? (char) p[i] : '.';
    }
    hex[16 * 3 - 1] = '\0';
    ascii[16] = '\0';
    ESP_LOGI(TAG, "SCAN 0x%04X: %s |%s|", addr + row * 16, hex, ascii);
  }

  this->memory_scan_chunk_index_++;
  return false;
}

void BentelKyo::publish_text_sensors_() {
  for (auto &entry : this->text_sensors_) {
    uint8_t idx = entry.index;

    switch (entry.type) {
      case TEXT_ZONE_TYPE: {
        if (idx >= (uint8_t) this->max_zones_) continue;
        const char *type_str;
        if (this->alarm_model_ == AlarmModel::KYO_32G) {
          // The panel's own string ROM at 0x34B1 enumerates the eight zone types in this
          // order, and the low three bits of the record's first byte index it. Confirmed on
          // KYO32G 2.13 by changing one zone from the keypad and re-reading: Instant kept
          // 0x?0, Duress produced 0x?4, 24 Hours produced 0x?3 — matching the ROM order.
          // The remaining bits are not the type (bit 4 tracks the wiring/balance group, as
          // on KYO8) so they are masked off rather than reported as "Unknown".
          static const char *const KYO32G_ZONE_TYPES[8] = {
              "Instant", "Delayed", "Path", "24 Hours", "Duress", "Fire", "Switch", "Arm Only"};
          type_str = KYO32G_ZONE_TYPES[this->zone_type_raw_[idx] & 0x07];
        } else {
          switch (this->zone_type_raw_[idx]) {
            case 0x00: type_str = "Instant"; break;
            case 0x01: type_str = "Delayed"; break;
            case 0x02: type_str = "Path"; break;
            case 0x18: type_str = "Unconfigured"; break;
            default: type_str = "Unknown"; break;
          }
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
      // On the KYO8 family the timers are decoded from the 0x00DC block for partitions
      // 1-4 (issue #113): entry/exit in seconds, siren duration in minutes (one global
      // byte at 0x00FA, published on each partition slot). Higher partition slots
      // publish "N/A" so a default 0 isn't mistaken for a real value.
      case TEXT_PARTITION_ENTRY_DELAY:
        if (idx >= KYO_MAX_PARTITIONS) continue;
        entry.sensor->publish_state(this->is_kyo8_family_() && idx >= KYO_PARTITIONS_8
                                        ? "N/A"
                                        : to_string(this->partition_entry_delay_[idx]) + "s");
        break;
      case TEXT_PARTITION_EXIT_DELAY:
        if (idx >= KYO_MAX_PARTITIONS) continue;
        entry.sensor->publish_state(this->is_kyo8_family_() && idx >= KYO_PARTITIONS_8
                                        ? "N/A"
                                        : to_string(this->partition_exit_delay_[idx]) + "s");
        break;
      case TEXT_PARTITION_SIREN_TIMER:
        if (idx >= KYO_MAX_PARTITIONS) continue;
        if (this->is_kyo8_family_()) {
          entry.sensor->publish_state(idx >= KYO_PARTITIONS_8
                                          ? "N/A"
                                          : to_string(this->partition_siren_timer_[idx]) + "min");
        } else {
          entry.sensor->publish_state(to_string(this->partition_siren_timer_[idx]));
        }
        break;
      case TEXT_PARTITION_NAME:
        if (idx >= KYO_MAX_PARTITIONS) continue;
        entry.sensor->publish_state(this->partition_name_[idx].empty() ? "N/A" : this->partition_name_[idx]);
        break;
      case TEXT_CODE_NAME:
        if (idx >= KYO_MAX_CODES) continue;
        entry.sensor->publish_state(this->code_name_[idx].empty() ? "N/A" : this->code_name_[idx]);
        break;
      case TEXT_PANEL_MODE_RAW: {
        char buf[8];
        snprintf(buf, sizeof(buf), "%02X %02X", this->panel_mode_raw_[0], this->panel_mode_raw_[1]);
        entry.sensor->publish_state(buf);
        break;
      }
      case TEXT_STATUS_FLAGS_RAW: {
        char buf[20];
        snprintf(buf, sizeof(buf), "%02X %02X %02X %02X %02X",
                 this->status_flags_raw_[0], this->status_flags_raw_[1],
                 this->status_flags_raw_[2], this->status_flags_raw_[3],
                 this->status_flags_raw_[4]);
        entry.sensor->publish_state(buf);
        break;
      }
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
