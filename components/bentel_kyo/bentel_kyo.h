/*
 * espkyogate - ESPHome native component for Bentel KYO Units
 * Project started and maintained by Lorenzo De Luca (me@lorenzodeluca.dev)
 * Special thanks for ESPHome native component refactor to Rui Marinho (ruipmarinho@gmail.com)
 *
 * GNU Affero General Public License v3.0
 */

#pragma once

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/core/time.h"
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
static const uint8_t KYO_PARTITIONS_8 = 4;  // KYO4/8 expose 4 partitions (see decode_event_code_)
static const uint8_t KYO_MAX_OUTPUTS = 16;
static const uint8_t KYO_OUTPUTS_8 = 5;  // KYO4/8 expose 5 outputs (status bits 0-4)
static const uint8_t KYO_MAX_KEYFOBS = 16;
static const uint8_t KYO_MAX_CODES = 24;

// Response sizes
static const int RESP_SENSOR_KYO32 = 18;
static const int RESP_SENSOR_KYO8 = 12;
static const int RESP_PARTITION_KYO32 = 26;
static const int RESP_PARTITION_KYO8 = 17;
static const int RESP_VERSION = 19;

// Communication health
static const int MAX_INVALID_COUNT = 3;
static const uint32_t SERIAL_TIMEOUT_MS = 300;
static const uint32_t INTER_BYTE_SILENCE_MS = 20;

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
  PANEL_PROGRAMMING_MODE,
  TROUBLE_ACTIVE,
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
  TEXT_PARTITION_NAME,
  TEXT_CODE_NAME,
  TEXT_PANEL_MODE_RAW,
  TEXT_STATUS_FLAGS_RAW,
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
  void read_event_log();
  void memory_scan();  // debug: dump unmapped config regions to the log (issue #113)
  void activate_output(uint8_t output_number);
  void deactivate_output(uint8_t output_number);
  void pulse_output(uint8_t output_number, uint32_t pulse_time_ms);
  void include_zone(uint8_t zone_number);
  void exclude_zone(uint8_t zone_number);
  void update_datetime(uint8_t day, uint8_t month, uint16_t year,
                       uint8_t hours, uint8_t minutes, uint8_t seconds);
  void sync_datetime_from_ntp();

  // Polling control
  void set_polling_enabled(bool enabled);
  bool is_polling_enabled() const { return this->polling_enabled_; }

  // Debug trace control
  void set_serial_trace(bool enabled) { this->serial_trace_ = enabled; }
  bool is_serial_trace_enabled() const { return this->serial_trace_; }
  void set_log_trace(bool enabled) { this->log_trace_ = enabled; }
  bool is_log_trace_enabled() const { return this->log_trace_; }

  // Re-read panel configuration registers
  void reread_config();

  friend class BentelKyoAlarmPanel;

 protected:
  // Protocol commands
  static constexpr uint8_t CMD_GET_SENSOR_STATUS[6] = {0xF0, 0x04, 0xF0, 0x0A, 0x00, 0xEE};
  // Partition status lives at a different register per firmware generation:
  // KYO32 non-G reads 0x14EC, while KYO32G (and KYO8W, per #109) read 0x1502.
  static constexpr uint8_t CMD_GET_PARTITION_KYO32[6] = {0xF0, 0xEC, 0x14, 0x12, 0x00, 0x02};
  static constexpr uint8_t CMD_GET_PARTITION_KYO32G[6] = {0xF0, 0x02, 0x15, 0x12, 0x00, 0x19};
  static constexpr uint8_t CMD_GET_PARTITION_KYO8[6] = {0xF0, 0x68, 0x0E, 0x09, 0x00, 0x6F};
  static constexpr uint8_t CMD_GET_VERSION[6] = {0xF0, 0x00, 0x00, 0x0B, 0x00, 0xFB};
  static constexpr uint8_t CMD_RESET_ALARMS[9] = {0x0F, 0x05, 0xF0, 0x01, 0x00, 0x05, 0xFF, 0x00, 0xFF};

  // Internal methods
  bool detect_alarm_model_(const uint8_t *rx, int count);
  bool parse_sensor_status_(const uint8_t *rx, int count);
  bool parse_partition_status_(const uint8_t *rx, int count);
  void send_command_async_(const uint8_t *cmd, int cmd_len, uint8_t pending_op, uint32_t timeout_ms = 200);
  void handle_serial_failure_();
  int send_message_(const uint8_t *cmd, int cmd_len, uint8_t *response, uint32_t timeout_ms = SERIAL_TIMEOUT_MS);
  int read_register_(uint16_t address, uint8_t length, uint8_t *response, uint32_t timeout_ms = SERIAL_TIMEOUT_MS);
  // Model-family helper (issue #113): KYO4/8/8G use a config memory map distinct from both
  // KYO32 (non-G) and KYO32G, so config-table addresses must branch three ways. KYO8W is
  // excluded — it uses KYO32-format runtime responses (issue #107/PR #109) and has no
  // validated KYO8 config trace; see the definition for details.
  bool is_kyo8_family_() const;
  // Selects the per-model base-address map for a config table. Returns nullptr when the
  // table location is not yet known for the current model, so the reader skips it instead
  // of decoding garbage from the wrong address.
  const uint16_t *select_name_bases_(const uint16_t *nong, const uint16_t *kyo32g,
                                     const uint16_t *kyo8) const;
  // Config reads are chunked: one 64-byte block per update() cycle, returning true
  // when the whole table is done, so a single cycle never blocks on multiple reads.
  bool read_zone_config_();
  bool read_zone_names_();
  bool read_zone_esn_next_();    // reads one zone ESN per call, returns true when done
  bool read_output_names_();
  void read_partition_config_();
  bool read_keyfob_esn_next_();  // reads one keyfob ESN per call, returns true when done
  bool read_keyfob_names_();
  bool read_partition_names_();
  bool read_code_names_();
  // Generic name-table reader: reads one block (up to 4 names) per call into `out`.
  bool read_name_table_chunk_(const uint16_t *bases, int num_blocks, int count,
                              std::string *out, const char *what);
  bool read_event_log_next_();  // reads one 64-byte chunk per call, returns true when done
  bool memory_scan_next_();     // reads one 64-byte chunk per call, returns true when done
  bool is_secret_address_(uint16_t address) const;   // true for bytes the scan masks as credentials
  static bool is_kyo32_secret_address_(uint16_t address);
  const char *decode_event_code_(uint16_t code, uint8_t *entity_out, char *buf, size_t buf_len);
  void read_panel_mode_();
  void read_status_flags_();
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
  uint32_t serial_timeout_ms_{200};
  // Callback: 0=detect, 1=sensor, 2=partition
  uint8_t serial_pending_op_{0};

  // Polling control
  bool polling_enabled_{true};

  // Debug trace flags (toggled at runtime via switches)
  bool serial_trace_{false};  // log TX/RX hex on serial commands
  bool log_trace_{false};     // log state changes (zone, partition, etc.)

  // Pulse output state (non-blocking timer-based)
  uint32_t pulse_output_end_ms_[KYO_MAX_OUTPUTS]{0};    // 0 = no pulse pending for this output

  // Communication health and backoff
  bool communication_ok_{false};
  uint32_t backoff_until_ms_{0};  // skip polling until this millis() value
  uint8_t consecutive_failures_{0};  // for exponential backoff (caps at 7 = 32s)

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

  // Panel Mode register (0x01E6, 2 bytes) — capture-validated idle baseline
  uint8_t panel_mode_raw_[2]{0x11, 0x10};
  bool panel_programming_mode_{false};

  // Status Flags register (0x1503, 5 bytes) — capture-validated no-trouble baseline
  uint8_t status_flags_raw_[5]{0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  bool trouble_active_{false};

  bool zone_bypass_[KYO_MAX_ZONES]{};
  bool zone_alarm_memory_[KYO_MAX_ZONES]{};
  bool zone_tamper_memory_[KYO_MAX_ZONES]{};

  // Zone configuration (read once from panel config registers, one step per update cycle)
  uint8_t config_read_step_{0};    // 0=not started, 1-12=reading, 13=done
  int config_chunk_index_{0};      // current 64-byte block within a chunked config read
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

  // Partition names (read once from 0x2BA0)
  std::string partition_name_[KYO_MAX_PARTITIONS];

  // Keyfob ESN (read once from 0xC0B1) and names (read once from 0x3180)
  std::string keyfob_esn_[KYO_MAX_KEYFOBS];
  std::string keyfob_name_[KYO_MAX_KEYFOBS];

  // Code names (read once from 0x3000)
  std::string code_name_[KYO_MAX_CODES];

  // Event log dump (on-demand via button)
  bool event_log_read_pending_{false};
  int event_log_chunk_index_{0};
  int event_log_entries_logged_{0};

  // Debug memory scan (on-demand via button, issue #113)
  bool memory_scan_pending_{false};
  int memory_scan_chunk_index_{0};
};

}  // namespace bentel_kyo
}  // namespace esphome
