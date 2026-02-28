/*
 * espkyogate - ESPHome component for Bentel KYO alarms
 * Copyright (C) 2025 Lorenzo De Luca (me@lorenzodeluca.dev)
 * Copyright (C) 2026 Rui Marinho (ruipmarinho@gmail.com)
 *
 * GNU Affero General Public License v3.0
 */

#pragma once

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/alarm_control_panel/alarm_control_panel.h"

#include <vector>
#include <string>
#include <cstring>

namespace esphome {
namespace bentel_kyo {

static const char *const TAG = "bentel_kyo";

static const uint8_t KYO_MAX_ZONES = 32;
static const uint8_t KYO_MAX_ZONES_8 = 8;
static const uint8_t KYO_MAX_PARTITIONS = 8;
static const uint8_t KYO_MAX_OUTPUTS = 16;
static const uint8_t KYO_MAX_KEYFOBS = 16;

// Response sizes
static const int RESP_SENSOR_KYO32 = 18;
static const int RESP_SENSOR_KYO8 = 12;
static const int RESP_PARTITION_KYO32 = 26;
static const int RESP_PARTITION_KYO8 = 17;
static const int RESP_VERSION = 19;

// Communication health
static const int MAX_INVALID_COUNT = 3;
static const uint32_t SERIAL_TIMEOUT_MS = 250;
static const uint32_t INTER_BYTE_SILENCE_MS = 10;

enum class AlarmModel : uint8_t {
  UNKNOWN = 0,
  KYO_4,
  KYO_8,
  KYO_8G,
  KYO_8W,
  KYO_32,
  KYO_32G,
};

enum BinarySensorType : uint8_t {
  ZONE = 0,
  ZONE_TAMPER,
  ZONE_BYPASS,
  ZONE_ALARM_MEMORY,
  ZONE_TAMPER_MEMORY,
  PARTITION_ALARM,
  WARNING_MAINS_FAILURE,
  WARNING_BPI_MISSING,
  WARNING_FUSE_FAULT,
  WARNING_LOW_BATTERY,
  WARNING_PHONE_LINE_FAULT,
  WARNING_DEFAULT_CODES,
  WARNING_WIRELESS_FAULT,
  TAMPER_ZONE,
  TAMPER_FALSE_KEY,
  TAMPER_BPI,
  TAMPER_SYSTEM,
  TAMPER_RF_JAM,
  TAMPER_WIRELESS,
  SIREN,
  COMMUNICATION,
  OUTPUT_STATE,
};

enum TextSensorType : uint8_t {
  TEXT_ZONE_TYPE = 0,
  TEXT_ZONE_NAME,
  TEXT_ZONE_AREA,
  TEXT_ZONE_ESN,
  TEXT_OUTPUT_NAME,
  TEXT_KEYFOB_ESN,
  TEXT_KEYFOB_NAME,
  TEXT_PARTITION_ENTRY_DELAY,
  TEXT_PARTITION_EXIT_DELAY,
  TEXT_PARTITION_SIREN_TIMER,
};

struct RegisteredTextSensor {
  text_sensor::TextSensor *sensor;
  TextSensorType type;
  uint8_t index;  // 0-based zone index
};

// Forward declarations
class BentelKyoAlarmPanel;

struct RegisteredBinarySensor {
  binary_sensor::BinarySensor *sensor;
  BinarySensorType type;
  uint8_t index;  // 0-based zone/partition/output index
};

// Async serial state machine states
enum class SerialState : uint8_t {
  IDLE = 0,
  WAITING_RESPONSE,
};

class BentelKyo : public PollingComponent, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_CONNECTION; }

  // Registration methods called from code generation
  void register_alarm_panel(BentelKyoAlarmPanel *panel);
  void register_binary_sensor(binary_sensor::BinarySensor *sensor, BinarySensorType type, uint8_t index);
  void set_firmware_version_text_sensor(text_sensor::TextSensor *sensor) { this->firmware_version_sensor_ = sensor; }
  void set_alarm_model_text_sensor(text_sensor::TextSensor *sensor) { this->alarm_model_sensor_ = sensor; }
  void register_text_sensor(text_sensor::TextSensor *sensor, TextSensorType type, uint8_t index);

  // Public command methods
  void arm_partition(uint8_t partition, uint8_t arm_type);
  void disarm_partition(uint8_t partition);
  void arm_all_partitions(uint8_t arm_type);
  void disarm_all_partitions();
  void arm_preset(uint8_t total_mask, uint8_t partial_mask, uint8_t partial_d0_mask);
  void reset_alarms();
  void activate_output(uint8_t output_number);
  void deactivate_output(uint8_t output_number);
  void include_zone(uint8_t zone_number);
  void exclude_zone(uint8_t zone_number);
  void update_datetime(uint8_t day, uint8_t month, uint16_t year,
                       uint8_t hours, uint8_t minutes, uint8_t seconds);

  // Polling control
  void set_polling_enabled(bool enabled);
  bool is_polling_enabled() const { return this->polling_enabled_; }

  // Re-read panel configuration registers
  void reread_config();

  friend class BentelKyoAlarmPanel;

 protected:
  // Protocol commands
  static constexpr uint8_t CMD_GET_SENSOR_STATUS[6] = {0xF0, 0x04, 0xF0, 0x0A, 0x00, 0xEE};
  static constexpr uint8_t CMD_GET_PARTITION_KYO32G[6] = {0xF0, 0x02, 0x15, 0x12, 0x00, 0x19};
  static constexpr uint8_t CMD_GET_PARTITION_KYO32[6] = {0xF0, 0xEC, 0x14, 0x12, 0x00, 0x02};
  static constexpr uint8_t CMD_GET_PARTITION_KYO8[6] = {0xF0, 0x68, 0x0E, 0x09, 0x00, 0x6F};
  static constexpr uint8_t CMD_GET_VERSION[6] = {0xF0, 0x00, 0x00, 0x0B, 0x00, 0xFB};
  static constexpr uint8_t CMD_RESET_ALARMS[9] = {0x0F, 0x05, 0xF0, 0x01, 0x00, 0x05, 0xFF, 0x00, 0xFF};

  // Internal methods
  bool detect_alarm_model_(const uint8_t *rx, int count);
  bool parse_sensor_status_(const uint8_t *rx, int count);
  bool parse_partition_status_(const uint8_t *rx, int count);
  void send_command_async_(const uint8_t *cmd, int cmd_len, uint8_t pending_op, uint32_t timeout_ms = 80);
  void handle_serial_failure_();
  int send_message_(const uint8_t *cmd, int cmd_len, uint8_t *response, uint32_t timeout_ms = SERIAL_TIMEOUT_MS);
  int read_register_(uint16_t address, uint8_t length, uint8_t *response, uint32_t timeout_ms = SERIAL_TIMEOUT_MS);
  void read_zone_config_();
  void read_zone_names_();
  bool read_zone_esn_next_();    // reads one zone ESN per call, returns true when done
  void read_output_names_();
  void read_partition_config_();
  bool read_keyfob_esn_next_();  // reads one keyfob ESN per call, returns true when done
  void read_keyfob_names_();
  void publish_text_sensors_();

  // Checksum helpers
  static uint8_t calculate_crc_(const uint8_t *cmd, int len);
  static uint8_t calculate_checksum_(const uint8_t *data, int offset, int len);

  // Bit extraction helpers
  bool get_zone_bit_32_(const uint8_t *rx, int base_offset, int zone_index);
  bool get_zone_bit_8_(const uint8_t *rx, int offset, int zone_index);

  // State publishing
  void publish_binary_sensors_();
  void publish_alarm_panels_();

  // Registered entities
  std::vector<BentelKyoAlarmPanel *> alarm_panels_;
  std::vector<RegisteredBinarySensor> binary_sensors_;
  std::vector<RegisteredTextSensor> text_sensors_;
  text_sensor::TextSensor *firmware_version_sensor_{nullptr};
  text_sensor::TextSensor *alarm_model_sensor_{nullptr};

  // Model and state
  AlarmModel alarm_model_{AlarmModel::UNKNOWN};
  bool model_detected_{false};
  int max_zones_{KYO_MAX_ZONES};
  char firmware_version_[14]{};

  // Async serial I/O state machine
  SerialState serial_state_{SerialState::IDLE};
  uint8_t serial_rx_buf_[255]{};
  int serial_rx_index_{0};
  int serial_cmd_len_{0};  // length of command sent (to detect echo end)
  uint32_t serial_sent_ms_{0};
  uint32_t serial_last_byte_ms_{0};
  uint32_t serial_timeout_ms_{80};
  // Callback: 0=detect, 1=sensor, 2=partition
  uint8_t serial_pending_op_{0};

  // Polling control
  bool polling_enabled_{true};

  // Communication health and backoff
  bool communication_ok_{false};
  uint32_t backoff_until_ms_{0};  // skip polling until this millis() value
  uint8_t consecutive_failures_{0};  // for exponential backoff (caps at 5 = 32s)

  // Response caches for change detection
  uint8_t sensor_cache_[32]{};
  uint8_t partition_cache_[32]{};
  int sensor_cache_len_{0};
  int partition_cache_len_{0};
  bool force_publish_{true};

  // Parsed state buffers (updated from responses)
  // Sensor status
  bool zone_state_[KYO_MAX_ZONES]{};
  bool zone_tamper_[KYO_MAX_ZONES]{};
  bool partition_alarm_[KYO_MAX_PARTITIONS]{};
  bool warn_mains_failure_{false};
  bool warn_bpi_missing_{false};
  bool warn_fuse_fault_{false};
  bool warn_low_battery_{false};
  bool warn_phone_line_fault_{false};
  bool warn_default_codes_{false};
  bool warn_wireless_fault_{false};
  bool tamper_zone_{false};
  bool tamper_false_key_{false};
  bool tamper_bpi_{false};
  bool tamper_system_{false};
  bool tamper_rf_jam_{false};
  bool tamper_wireless_{false};

  // Partition status
  bool partition_armed_total_[KYO_MAX_PARTITIONS]{};
  bool partition_armed_partial_[KYO_MAX_PARTITIONS]{};
  bool partition_armed_partial_delay0_[KYO_MAX_PARTITIONS]{};
  bool partition_disarmed_[KYO_MAX_PARTITIONS]{};
  bool siren_active_{false};
  bool output_state_[KYO_MAX_OUTPUTS]{};
  bool zone_bypass_[KYO_MAX_ZONES]{};
  bool zone_alarm_memory_[KYO_MAX_ZONES]{};
  bool zone_tamper_memory_[KYO_MAX_ZONES]{};

  // Zone configuration (read once from panel config registers, one step per update cycle)
  uint8_t config_read_step_{0};    // 0=not started, 1-8=reading, 9=done
  uint16_t text_sensor_republish_counter_{0};  // re-publish text sensors every 120 cycles (~60s)
  int esn_read_index_{0};          // current zone index for per-zone ESN reads
  int keyfob_read_index_{0};       // current keyfob index for per-slot ESN reads
  uint8_t zone_type_raw_[KYO_MAX_ZONES]{};   // raw type byte
  uint8_t zone_area_mask_[KYO_MAX_ZONES]{};   // area bitmask
  bool zone_enrolled_[KYO_MAX_ZONES]{};
  std::string zone_name_[KYO_MAX_ZONES];
  std::string zone_esn_[KYO_MAX_ZONES];

  // Output names (read once from 0x3280)
  std::string output_name_[KYO_MAX_OUTPUTS];

  // Partition configuration (read once from 0x01E9)
  uint8_t partition_entry_delay_[KYO_MAX_PARTITIONS]{};
  uint8_t partition_exit_delay_[KYO_MAX_PARTITIONS]{};
  uint8_t partition_siren_timer_[KYO_MAX_PARTITIONS]{};

  // Keyfob ESN (read once from 0xC0B1) and names (read once from 0x3180)
  std::string keyfob_esn_[KYO_MAX_KEYFOBS];
  std::string keyfob_name_[KYO_MAX_KEYFOBS];
};

}  // namespace bentel_kyo
}  // namespace esphome
