# ESPKyoGate - ESPHome Component for Bentel KYO Alarms

[![](https://img.shields.io/static/v1?label=Sponsor&message=%E2%9D%A4&logo=GitHub&color=%23fe8e86)](https://github.com/sponsors/lorenzo-deluca)
[![buy me a coffee](https://img.shields.io/badge/support-buymeacoffee-222222.svg?style=flat-square)](https://www.buymeacoffee.com/lorenzodeluca)

A proper [ESPHome external component](https://esphome.io/components/external_components.html) for **Bentel Security KYO alarm panels**, based on ESP32 and communicating over RS-232 serial via a MAX3232 level shifter.

> **Warning**
> This software was developed by analyzing serial messages from/to the panel. It is not sponsored or officially supported by **Bentel**.

## Supported Models

- [x] Bentel KYO 32G
- [x] Bentel KYO 32 / 32M (non-G)
- [x] Bentel KYO 8G
- [x] Bentel KYO 8W
- [x] Bentel KYO 8
- [x] Bentel KYO 4
> **Warning**
> :warning: This software was developed by analyzing serial messages from/to central, it was not sponsored or officially supported by **Bentel**
> If someone from **Bentel** would like to contribute or collaborate please contact me at [me@lorenzodeluca.dev](mailto:me@lorenzodeluca.dev?subject=[GitHub]ESPKyoGate)

## Contents
- [Community Forum & Support](#community-forum--support)
- [Hardware Connections](#hardware-connections)
- [ESPHome Preparation](#esphome-preparation)
- [Build and Upload Firmware](#build-and-upload-firmware)
- [Home Assistant Integration](#home-assistant-integration)
- [Alarm Control Panel Package](#alarm-control-panel-package)
- [Troubleshooting - FAQ](#troubleshooting-faq)

## Community Forum & Support
For discussions, questions, and community support, you can join the conversation on the official Home Assistant Community forum.\
➡️ [**Join the discussion here: Bentel KYO32 Alarm System Integration**](https://community.home-assistant.io/t/bentel-kyo32-alarm-system-integration)

## Hardware Connections
I strongly recommend using an **ESP32** Based board, like this one https://it.aliexpress.com/item/4001340660273.html.

Model detection is automatic via firmware version query on first connection.

## Features

- **Alarm Control Panel** entities per partition (arm away, arm home, arm night, disarm) with full Home Assistant integration
- **Binary sensors** for zones, zone tamper, zone bypass, alarm memory, tamper memory, warnings, tamper flags, siren, communication status, and output states
- **Text sensors** for firmware version, alarm model, zone diagnostics (type, panel name, partition, serial number), output names, partition timers, keyfob serial numbers, partition names, and code names
- **Non-blocking serial I/O** with async state machine (no blocking delays)
- **Response caching** with change detection (only publishes when state changes)
- **Exponential backoff** on communication failures (2s to 32s)
- **Dual-query polling** (sensor + partition status every 500ms cycle)
- **One-time config reads** for zone configuration, names, serial numbers, output names, partition timers, keyfob serial numbers, partition names, and code names

## Hardware

### Components

- ESP32 board (e.g., [ESP32 DevKit](https://it.aliexpress.com/item/4001340660273.html))
- [MAX3232 RS-232 to TTL adapter](https://it.aliexpress.com/item/696400942.html) with DB9 connector
- 12V to USB converter for panel-powered operation (recommended for battery backup)

### Wiring

| ESP32 Pin | MAX3232 | KYO Panel |
|-----------|---------|-----------|
| GPIO17 (TX) | TX | RS-232 RX |
| GPIO16 (RX) | RX | RS-232 TX |
| 5V | VCC | - |
| GND | GND | GND |

Power the ESP32 from the panel's +B/AUX 12V output via a 12V-to-USB converter. This ensures the ESP32 stays powered during mains failures (panel battery backup).

<img src="images/wiring_esp32.png" alt="ESP32-Wiring" width="450px"/>

## Installation

### As an External Component (recommended)

Add to your ESPHome YAML:

```yaml
external_components:
  - source: github://lorenzo-deluca/espkyogate
    components: [bentel_kyo]
```

### Local Installation

Clone this repository and reference the components directory:

```yaml
external_components:
  - source:
      type: local
      path: path/to/espkyogate/components
    components: [bentel_kyo]
```

## Configuration

### Minimal Example

```yaml
external_components:
  - source: github://lorenzo-deluca/espkyogate
    components: [bentel_kyo]

uart:
  id: uart_bus
  tx_pin: GPIO17
  rx_pin: GPIO16
  baud_rate: 9600
  data_bits: 8
  parity: EVEN

bentel_kyo:
  id: kyo
  uart_id: uart_bus

alarm_control_panel:
  - platform: bentel_kyo
    bentel_kyo_id: kyo
    name: "House"
    partition: 1
```

### Full Example

```yaml
external_components:
  - source: github://lorenzo-deluca/espkyogate
    components: [bentel_kyo]

esp32:
  board: lolin32
  framework:
    type: esp-idf

uart:
  id: uart_bus
  tx_pin: GPIO17
  rx_pin: GPIO16
  baud_rate: 9600
  data_bits: 8
  parity: EVEN

bentel_kyo:
  id: kyo
  uart_id: uart_bus

# Alarm Control Panel — one per partition (up to 8)
# Maps to Home Assistant alarm_control_panel entities with arm/disarm controls

alarm_control_panel:
  - platform: bentel_kyo
    bentel_kyo_id: kyo
    name: "Ground Floor"
    partition: 1
    entry_delay:
      name: "Ground Floor Entry Delay"
    exit_delay:
      name: "Ground Floor Exit Delay"
    siren_timer:
      name: "Ground Floor Siren Timer"

  - platform: bentel_kyo
    bentel_kyo_id: kyo
    name: "First Floor"
    partition: 2

  - platform: bentel_kyo
    bentel_kyo_id: kyo
    name: "Basement"
    partition: 3

# Binary Sensors — zones, warnings, tamper, siren, outputs

binary_sensor:
  - platform: bentel_kyo
    bentel_kyo_id: kyo

    # Zones (1-32) — zone status with optional diagnostic text sensors
    zones:
      - zone: 1
        name: "Front Door"
        device_class: door
        zone_type:
          name: "Front Door Zone Type"
        panel_name:
          name: "Front Door Panel Name"
        zone_partition:
          name: "Front Door Partition"
        serial_number:
          name: "Front Door Serial Number"
      - zone: 2
        name: "Living Room PIR"
        device_class: motion
      - zone: 3
        name: "Kitchen PIR"
        device_class: motion

    # Zone tamper (per-zone tamper detection)
    zone_tamper:
      - zone: 1
        name: "Front Door Tamper"
      - zone: 2
        name: "Living Room Tamper"

    # Zone bypass (per-zone exclusion status)
    zone_bypass:
      - zone: 1
        name: "Front Door Bypassed"

    # Zone alarm memory
    zone_alarm_memory:
      - zone: 1
        name: "Front Door Alarm Memory"

    # Zone tamper memory
    zone_tamper_memory:
      - zone: 1
        name: "Front Door Tamper Memory"

    # Warning flags
    warnings:
      mains_failure:
        name: "Mains Power Failure"
      bpi_missing:
        name: "BPI Module Missing"
      fuse_fault:
        name: "Fuse Fault"
      low_battery:
        name: "Low Battery"
      phone_line_fault:
        name: "Phone Line Fault"
      default_codes:
        name: "Default Codes Warning"
      wireless_fault:
        name: "Wireless Fault"

    # Tamper flags
    tamper:
      tamper_zone:
        name: "Zone Tamper"
      tamper_false_key:
        name: "False Key Tamper"
      tamper_bpi:
        name: "BPI Tamper"
      tamper_system:
        name: "System Tamper"
      tamper_rf_jam:
        name: "RF Jam Detected"
      tamper_wireless:
        name: "Wireless Tamper"

    # Communication status (connectivity to panel)
    communication:
      name: "Panel Communication"

    # Siren status
    siren:
      name: "Siren"

    # OC output states (1-8)
    output_state:
      - output_number: 1
        name: "Output 1"
        panel_name:
          name: "Output 1 Panel Name"
      - output_number: 2
        name: "Output 2"

# Text Sensors — firmware, model, keyfobs

text_sensor:
  - platform: bentel_kyo
    bentel_kyo_id: kyo
    firmware_version:
      name: "Firmware Version"
    alarm_model:
      name: "Alarm Model"
    keyfobs:
      - slot: 1
        name: "Keyfob 1"
      - slot: 2
        name: "Keyfob 2"
    partitions:
      - partition: 1
        name: "Partition 1 Name"
      - partition: 2
        name: "Partition 2 Name"
    codes:
      - code: 1
        name: "Code 1 Name"
      - code: 2
        name: "Code 2 Name"

# Switch — polling control

switch:
  - platform: bentel_kyo
    bentel_kyo_id: kyo
    name: "Panel Polling"

# Buttons — reread config, reset alarms, event log, arm presets

button:
  - platform: bentel_kyo
    bentel_kyo_id: kyo
    type: reread_config
    name: "Reread Panel Config"
  - platform: bentel_kyo
    bentel_kyo_id: kyo
    type: reset_alarms
    name: "Reset Alarms"
  - platform: bentel_kyo
    bentel_kyo_id: kyo
    type: read_event_log
    name: "Read Event Log"
  - platform: bentel_kyo
    bentel_kyo_id: kyo
    type: arm_preset
    name: "Arm All Away"
    partitions:
      1: away
      2: away
      3: away
  - platform: bentel_kyo
    bentel_kyo_id: kyo
    type: arm_preset
    name: "Disarm All"
    partitions:
      1: disarm
      2: disarm
      3: disarm
```

## Alarm Control Panel

Each `alarm_control_panel` entry creates a proper Home Assistant alarm panel entity for a partition:

| HA Action | KYO Command | Description |
|-----------|-------------|-------------|
| Arm Away | Total arm | Full perimeter + interior protection |
| Arm Home | Partial arm | Bypasses zones with the "Internal" attribute |
| Arm Night | Partial arm delay 0 | Bypasses "Internal" zones AND removes entry delay |
| Disarm | Disarm | Disarm the partition |

The partition state is read from the panel every 500ms and mapped to:

| Panel State | HA State |
|-------------|----------|
| Disarmed | `disarmed` |
| Partition alarm | `triggered` |
| Armed total | `armed_away` |
| Armed partial | `armed_home` |
| Armed partial delay 0 | `armed_night` |

> **Note**: Disarmed takes priority over triggered. The panel's alarm bit persists after disarming until the alarm memory is explicitly reset. Once the partition is disarmed, the alarm has been acknowledged and the state shows `disarmed`.

Optional diagnostic text sensors for each partition:
- `entry_delay` — entry delay timer (seconds)
- `exit_delay` — exit delay timer (seconds)
- `siren_timer` — siren duration timer

### Code Protection

You can require a PIN code to arm or disarm from Home Assistant. The code is validated by the ESP32 before the command is sent to the panel — it does not need to match the panel's own installer/user code.

```yaml
alarm_control_panel:
  - platform: bentel_kyo
    bentel_kyo_id: kyo
    name: "House"
    partition: 1
    codes:
      - !secret alarm_code
    requires_code_to_arm: false
```

| Option | Default | Description |
|--------|---------|-------------|
| `codes` | (none) | List of valid PIN codes (strings). Use `!secret` to avoid plaintext in YAML |
| `requires_code_to_arm` | `false` | Require code entry to arm the partition |

When codes are configured, **disarm always requires a valid code**. HA automatically shows a numeric keypad on the alarm panel card. Arming does not require a code by default — set `requires_code_to_arm: true` to also require it for arm actions.

> **Note**: Code protection only applies to the `alarm_control_panel` entities in HA. Arm preset buttons bypass code validation — they call the hub directly.

### Arming Modes and the "Internal" Zone Attribute

The KYO panel supports three arming modes per partition (from the Bentel user manual, Table 6):

| Mode | Panel Letter | Behavior |
|------|:-----------:|----------|
| **Away** | A | Monitors **all zones** in the partition |
| **Stay** (Home) | S | Bypasses zones with the **Internal** attribute (called "Interna" in KyoUnit Italian, "Stay" in the English user manual) |
| **Stay-0-Delay** (Night) | I | Same as Stay, but also **removes the entry delay** — all zones fire instantly |

The **Internal** attribute is configured per-zone in KyoUnit (or via keypad parameter 164-192). Only zones with this attribute checked are bypassed during Stay/Night modes. Away mode always monitors all zones regardless of the attribute.

If **no zones** have the Internal attribute set, then Stay and Night modes behave identically to Away — the only difference being that Night removes the entry delay. This is useful when you want full protection with instant response (e.g., arming at night and using a keyfob to disarm before moving).

### Zone Types

| Type | KyoUnit (Italian) | Behavior |
|------|-------------------|----------|
| **Instant** | Immediata | Triggers alarm immediately |
| **Path** | Percorso | Starts the partition's entry delay timer |
| **Delayed** | Ritardata | Follows an active entry delay (if a Path zone triggered first); otherwise instant |
| **24h** | 24h | Always active, even when disarmed |

Path zones are typically used on entry/exit routes (e.g., the hallway from the main door to the keypad). In Night mode, the entry delay is removed, so Path zones fire instantly like all other zones.

## Buttons

```yaml
button:
  - platform: bentel_kyo
    bentel_kyo_id: kyo
    type: reread_config
    name: "Reread Panel Config"
```

### Button Types

| Type | Description |
|------|-------------|
| `reread_config` | Re-read zone configuration, names, serial numbers from panel |
| `reset_alarms` | Reset alarm memory on the panel |
| `read_event_log` | Read panel event log (256 entries) and dump to ESPHome logs |
| `arm_all_away` | Arm all registered partitions in Away mode |
| `arm_all_home` | Arm all registered partitions in Home/Stay mode |
| `arm_all_night` | Arm all registered partitions in Night mode |
| `disarm_all` | Disarm all registered partitions |
| `arm_preset` | Arm/disarm specific partitions with per-partition mode selection |

### Arm Preset Buttons

The `arm_preset` button type lets you define a per-partition arming configuration that executes as a single command. This is the recommended way to implement arming scenes — one button press, one command, no race conditions.

```yaml
button:
  # Everyone leaves the house — arm everything
  - platform: bentel_kyo
    bentel_kyo_id: kyo
    type: arm_preset
    name: "Everyone Out"
    icon: "mdi:shield-lock"
    partitions:
      1: away
      2: away
      3: away

  # Going to sleep — disarm upstairs, arm ground floor + basement
  - platform: bentel_kyo
    bentel_kyo_id: kyo
    type: arm_preset
    name: "Sleeping Rooms Free"
    icon: "mdi:shield-home"
    partitions:
      1: disarm
      2: away
      3: away

  # Going to sleep — everything instant, disarm via keyfob before leaving room
  - platform: bentel_kyo
    bentel_kyo_id: kyo
    type: arm_preset
    name: "Sleeping Instant"
    icon: "mdi:shield-moon"
    partitions:
      1: night
      2: night
      3: night

  # Disarm everything
  - platform: bentel_kyo
    bentel_kyo_id: kyo
    type: arm_preset
    name: "Disarm All"
    icon: "mdi:shield-off"
    partitions:
      1: disarm
      2: disarm
      3: disarm
```

Available modes per partition:

| Mode | Description |
|------|-------------|
| `away` | Full arm — monitors all zones |
| `home` / `stay` | Partial arm — bypasses Internal zones |
| `night` | Partial arm with no entry delay — bypasses Internal zones, all zones instant |
| `disarm` | Disarm the partition |

Partitions **not listed** in the config are sent as `disarm` (all mask bits not explicitly set remain `0`). Use this when you want a full deterministic preset that sets all partitions in one command.

### Using Preset Buttons in Home Assistant Automations

Preset buttons are the recommended way to create arming automations — they send a single command with all partition modes set simultaneously:

```yaml
# Home Assistant automation example
automation:
  - alias: "Arm when everyone leaves"
    trigger:
      - platform: state
        entity_id: group.family
        to: "not_home"
    action:
      - service: button.press
        target:
          entity_id: button.everyone_out
```

### Why Presets Instead of Alarm Control Panel Calls?

The `alarm_control_panel` entities control **one partition at a time**. To arm 3 partitions, you'd need 3 separate service calls, each sending a command to the panel. This creates race conditions — the second command may overwrite the first before the panel processes it.

Preset buttons send **one command** with all partition modes simultaneously. Use the alarm control panel entities for viewing partition state on dashboards and for occasional per-partition ad-hoc control.

## Switch

The polling switch enables or disables the component's serial polling loop:

```yaml
switch:
  - platform: bentel_kyo
    bentel_kyo_id: kyo
    name: "Panel Polling"
```

When disabled, the component stops querying the panel. Useful for temporarily freeing the serial port (e.g., when connecting KyoUnit for programming).

## Binary Sensor Reference

### Zones (`zones`)

| Key | Required | Description |
|-----|----------|-------------|
| `zone` | Yes | Zone number (1-32) |
| `name` | Yes | Entity name |
| `device_class` | No | HA device class (motion, door, window, etc.) |
| `zone_type` | No | Text sensor: zone type (Instant, Delayed, Path) |
| `panel_name` | No | Text sensor: zone name configured on the panel |
| `zone_partition` | No | Text sensor: which partition(s) the zone belongs to |
| `serial_number` | No | Text sensor: wireless sensor serial number |

### Zone Tamper (`zone_tamper`)

Per-zone tamper detection. Uses `device_class: tamper` by default.

### Zone Bypass (`zone_bypass`)

Per-zone exclusion/bypass status.

### Zone Alarm Memory (`zone_alarm_memory`)

Per-zone alarm memory (set after alarm, cleared on reset).

### Zone Tamper Memory (`zone_tamper_memory`)

Per-zone tamper memory.

### Warnings (`warnings`)

| Key | Device Class | Description |
|-----|-------------|-------------|
| `mains_failure` | power | Mains power lost |
| `bpi_missing` | problem | BPI bus module missing |
| `fuse_fault` | problem | Fuse blown |
| `low_battery` | battery | Panel battery low |
| `phone_line_fault` | problem | Telephone line fault |
| `default_codes` | problem | Default codes still active |
| `wireless_fault` | problem | Wireless module fault |

### Tamper Flags (`tamper`)

| Key | Description |
|-----|-------------|
| `tamper_zone` | Zone tamper detected |
| `tamper_false_key` | False key attempt |
| `tamper_bpi` | BPI bus tamper |
| `tamper_system` | System tamper |
| `tamper_rf_jam` | RF jamming detected |
| `tamper_wireless` | Wireless tamper |

### Other Sensors

| Key | Description |
|-----|-------------|
| `communication` | Panel serial communication status |
| `siren` | Siren active status |
| `output_state` | OC output state (1-8), with optional `panel_name` text sensor |

## Text Sensor Reference

| Key | Description |
|-----|-------------|
| `firmware_version` | Panel firmware version string |
| `alarm_model` | Detected alarm model (KYO4, KYO8, KYO32, etc.) |
| `keyfobs` | Keyfob serial numbers and names (slot 1-16) |
| `partitions` | Partition names as configured on the panel (partition 1-8) |
| `codes` | User code names as configured on the panel (code 1-24) |

## KYO32 vs KYO32G

| Feature | KYO32G | KYO32 (non-G) |
|---------|--------|----------------|
| Output readback | Real state | Always reads 0xFF |
| Max outputs | 8 | 3 |
| Wireless | Yes | Varies by sub-model |

On KYO32 non-G, output state sensors will not reflect the actual output state. Activate/deactivate commands still work correctly.

## Troubleshooting

- Exit the panel's programming mode before connecting (serial port is exclusive)
- Verify TX/RX wiring (try swapping if no response)
- Check MAX3232 module power supply
- Set `logger: level: DEBUG` for detailed serial communication logs
- KYO32G requires firmware **2.13** or later
- If communication drops, the component uses exponential backoff (2s-32s) before retrying

## Community

For discussions and support, visit the [Home Assistant Community forum](https://community.home-assistant.io/t/bentel-kyo32-alarm-system-integration).

## License

GNU AGPLv3 - [Lorenzo De Luca](https://lorenzodeluca.dev), [Rui Marinho](https://github.com/ruimarinho)

Original project by Lorenzo De Luca. Thanks to @dario81 for initial porting to ESPHome and @lcavalli for code refactor inspiration.
