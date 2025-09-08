# ESPhome for BENTEL KYO Units
[![](https://img.shields.io/static/v1?label=Sponsor&message=%E2%9D%A4&logo=GitHub&color=%23fe8e86)](https://github.com/sponsors/lorenzo-deluca)
[![buy me a coffee](https://img.shields.io/badge/support-buymeacoffee-222222.svg?style=flat-square)](https://www.buymeacoffee.com/lorenzodeluca)

Serial Bridge for **Bentel Kyo32G Alarm Central**, based on **ESP32** Board and **ESPHome** Open Source Firmware.
Thanks to @dario81 for initial porting to ESPHome and to @lcavalli for code refactor inspiration ;)

If you like this project you can support me with :coffee: , with **GitHub Sponsor** or simply put a :star: to this repository :blush:

[![](https://img.shields.io/static/v1?label=Sponsor&message=%E2%9D%A4&logo=GitHub&color=%23fe8e86)](https://github.com/sponsors/lorenzo-deluca)
<a href="https://www.buymeacoffee.com/lorenzodeluca" target="_blank">
  <img src="https://www.buymeacoffee.com/assets/img/custom_images/yellow_img.png" alt="Buy Me A Coffee" width="150px">
</a>

> **Warning**
> :warning: This software was developed by analyzing serial messages from/to central, it was not sponsored or officially supported by **Bentel**
> If someone from **Bentel** would like to contribute or collaborate please contact me at [me@lorenzodeluca.dev](mailto:me@lorenzodeluca.dev?subject=[GitHub]ESPKyoGate)


---
> <div align="center" style="background-color: #fffbe6; border: 2px solid #ffd166; border-radius: 10px; padding: 1.2em; margin-bottom: 2em;">
> <h2>⚠️ IMPORTANT DISCLAIMER — ESPHome 2025.7.0 and Above ⚠️</h2>
>
> 🚨 **From ESPHome version 2025.7.0 onwards, you may encounter compilation errors if you do not strictly follow all instructions and workarounds provided in this README and in the example YAML configuration.**
>
> - 🛠️ **Be sure to adapt your configuration by following the provided YAML file instructions.**
> - 📄 **Carefully read and implement every step for a successful compilation and operation.**
> - ❗ Any issues resulting from not following these steps will not be handled as bugs by the project maintainer.
>
> If you have trouble, please refer to the **Troubleshooting** section before opening new issues.
> </div>
---

### Tested on Kyo Unit
- [x] Bentel Kyo 32G
- [x] Bentel Kyo 32
- [x] Bentel Kyo 8W
- [x] Bentel Kyo 8WG
- [x] Bentel Kyo 8 (*)
- [x] Bentel Kyo 4 (*)
- [ ] If you have another Bentel Kyo unit test and let me know :)

**(*)**
For **Kyo 8** (no W or WG) and **Kyo 4** you have to use the **bentel_kyo4.h** file and **espkyogate_configuration_kyo4.yaml** for configuration reference!

## Contents
- [Community Forum & Support](#community-forum--support)
- [Hardware Connections](#hardware-connections)
- [ESPHome Preparation](#esphome-preparation)
- [Build and Upload Firmware](#build-and-upload-firmware)
- [Home Assistant Integration](#home-assistant-integration)
- [Troubleshooting - FAQ](#troubleshooting-faq)

## Community Forum & Support
For discussions, questions, and community support, you can join the conversation on the official Home Assistant Community forum.\
➡️ [**Join the discussion here: Bentel KYO32 Alarm System Integration**](https://community.home-assistant.io/t/bentel-kyo32-alarm-system-integration)

## Hardware Connections
I strongly recommend using an **ESP32** Based board, like this one https://it.aliexpress.com/item/4001340660273.html.

<img src="images/wiring_esp32.png" alt="ESP32-Wiring" width="450px"/>

In order to connect to the serial port of the KYO Unit I recommend a connector based on [**MAX3232**](https://it.aliexpress.com/item/696400942.html) module chip with DB9 connector.\
This connector should be connected to the classic **TX/RX of the ESP board** and to the power supply (GND, 5V) of the ESP32 board.

<img src="images/BentelKyo32Unit.jpg" alt="Central-Connections" width="650px"/>

The ESP32 board can be powered with USB directly from the 12V output of the control unit (the +/- pins on the lower left, powering the sensors) by connecting any 12V->USB converter.\
Like this one: 
https://www.amazon.it/FTVOGUE-Regolatore-Trasformatore-Caricabatterie-smartphone/dp/B07NQKBRG1/

Which I recommend because in this way, even in case of power failure, the ESP is powered by the control unit's battery.\
For connecting the power supply to the control unit I recommend taking the power supply from the terminal for auxiliary devices (+B/AUX).\
Negative/common can be taken from any negative.

<img src="images/KyoManual_Connections.png" alt="KyoManual-Connections" height="600"/>

## ESPHome Preparation
I suggest using the file `espkyogate_configuration.yaml` as a template and put your customizations there.

* Set `esphome` and `uart` settings depending on the board you use; example file is for ESP32 Devkit 1 board with included, commented settings for the older Wemos D1 mini.\
* Set `name`, `friendly_name` (how it will be presented in HA).
* Edit `binary_sensors` to configure how you want to present the sensors to Home Assistant.
  * All inputs have to be declared in both `lambda` and `binary_sensors`. Add only the ones you need to minimize overhead and complexity.
  * Make sure the order is respected between the two lists

Appropriate device classes are (among all device classes supported by Home Assistant):

| Device class | Home Assistant icons                                                                                          |
| ------------ | :-----------------------------------------------------------------------------------------------------------: |
| motion       | ![mdi-walk](images/icons/mdi-walk.png) ![mdi-run](images/icons/mdi-run.png)                                   |
| window       | ![mdi-window-closed](images/icons/mdi-window-closed.png) ![mdi-window-open](images/icons/mdi-window-open.png) |
| door         | ![mdi-door-closed](images/icons/mdi-door-closed.png) ![mdi-door](images/icons/mdi-door.png)                   |
| garage_door  | ![mdi-garage](images/icons/mdi-garage.png) ![mdi-garage-open](images/icons/mdi-garage-open.png)               |
| smoke        | ![mdi-smoke-detector-alert](images/icons/smoke-detector-alert.png)                                            |

Finally, create a `secrets.yaml` file with the following contents:
```yaml
wifi_ssid: "<your-wifi-ssid>"
wifi_password: "<your-wifi-password>"
hotspot_wifi_ssid: "<fallback-hotspot-wifi-ssid>"
hotspot_wifi_password: "<fallback-hotspot-wifi-password>"
ota_password: "<your-ota-password>"
api_encryption_key: "<your-encryption-key>"
```

* Populate `wifi_ssid` and `wifi_password` with details on how to connect to your network.
* Choose and write a `hotspot_wifi_ssid` and `hotspot_wifi_password` so the board can create a fallback hotspot in case of a failure of the main wifi network.
* Write a random password in `ota_password`, which will be used to update the board remotely.
* Generate an encryption key from [ESPHome](https://esphome.io/components/api.html) and set it in `api_encryption_key`


### Build and Upload Firmware

#### With ESPHome 
This way is the easiest, just copy the files from this repository to the esphome folder, edit the `espkyogate_configuration.yaml` file as above, upload and see if everything works from the logs.
You should see something similar:

<img src="images/ESPHomeLogs.png" alt="ESPHomeLogs" />

The [ESPHome interface](https://web.esphome.io/) can help if you never used it before. 

#### From esphome command line
`python3 -m esphome compile espkyogate_configuration.yaml`

`python3 -m esphome run espkyogate_configuration.yaml`

The above command uploads the new firmware and automatically waits for a connection to read logs. It's strongly suggested to set the `logger.level` to `DEBUG` at the first run to troubleshoot connection mistakes. Then set it back to `INFO` once it's stable.

##### Check logs
See logs with this command 
`python3 -m esphome logs espkyogate_configuration.yaml`

Make sure you always connect OTA instead of with the serial to USB port becaues it might be disabled due to a [bug](https://github.com/lorenzo-deluca/espkyogate/issues/16).

# Home Assistant Integration
If everything went well now you should find a new autodiscovered device in Home Assistant, called **Allarm**.

All sensors configured in `espkyogate_configuration.yaml` will be automatically created and associated to the device.

<img src="images/HomeAssistant-Lovelace.png" alt="Lovelace card"/>

## Avaiable Services
These methods will be available in the services

### Area Arm
``` yaml
service: esphome.espkyogate_arm_area
data:
  arm_type: 1 (total arm) - 2 (partially arm)
  area: <area_number>
  specific_area: 1 (arm only <area_number> without changing the others) - 0 (arm only <area_number> and disarm others)
```

### Area Disarm
``` yaml
service: esphome.espkyogate_disarm_area
data:
  area: <area_number>
  specific_area: 0 (disarm all areas) - 1 (disarm only <area_number> without changing the others)
```

### Zone Include
``` yaml
service: esphome.espkyogate_include_zone
data:
  zone_number: <zone_number>
```

### Zone Exclude
``` yaml
service: esphome.espkyogate_exclude_zone
data:
  zone_number: <zone_number>
```

### Reset Alarm Memory
``` yaml
service: esphome.espkyogate_reset_alarms
data: {}
```

### Activate Output
If an output is configured as 'Remote Command' (Comando Remoto) you can Activate or Deactivate
``` yaml
service: esphome.espkyogate_activate_output
data:
  output_number: <output_number>
```

### Deactivate Output
If an output is configured as 'Remote Command' (Comando Remoto) you can Activate or Deactivate
``` yaml
service: esphome.espkyogate_deactivate_output
data:
  output_number: <output_number>
```

### Pulse Output
If an output is configured as 'Remote Command' (Comando Remoto) you can use as Pulse Signal: form Inactive to Active and Inactive after some time
You can choose how many milliseconds the signal should remain high level.
``` yaml
service: esphome.espkyogate_pulse_output
data:
  output_number: <output_number>
  pulse_time: <pulse_time>
```

### Update Unit Date and Time
You can update the Kyo Date and Time internal Clock

``` yaml
service: esphome.espkyogate_update_datetime
data:
  day: 22
  month: 12
  year: 2022
  hours: 18
  minutes: 15
  seconds: 00
```

### Arm more than one area
If you want to arm several areas at the same time you have to call the same service several times, introducing a delay between one call and the next.
Below is an example of a script that arms two areas.
``` yaml
alias: Bentel Arma Fuori Casa
sequence:
  - service: esphome.espkyogate_arm_area
    data:
      area: 1
      arm_type: 1
      specific_area: 1
  - wait_template: ''
    timeout: '00:00:05'
  - service: esphome.espkyogate_arm_area
    data:
      area: 2
      arm_type: 1
      specific_area: 1
  - wait_template: ''
    timeout: '00:00:05'
  - service: esphome.espkyogate_arm_area
    data:
      area: 3
      arm_type: 1
      specific_area: 1
mode: single
```

## Create Lovelace Panel

Here the code to build Panel show above
``` yaml
type: vertical-stack
title: Allarme Bentel Casa
cards:
  - type: horizontal-stack
    cards:
      - type: button
        name: Arma in casa
        tap_action:
          action: call-service
          service: esphome.espkyogate_arm_area
          service_data:
            arm_type: 1
            area: 3
            specific_area: 1
        show_state: true
        show_icon: true
        show_name: true
        icon: mdi:shield-home-outline
        icon_height: 25px
      - type: button
        name: Arma Fuori Casa
        tap_action:
          action: call-service
          service: script.bentel_arma_fuori_casa
          service_data: {}
          target: {}
        show_icon: true
        show_state: true
        icon: mdi:shield-lock-outline
        icon_height: 25px
      - type: button
        name: Disarma
        tap_action:
          action: call-service
          service: esphome.espkyogate_disarm_area
          service_data:
            area: 1
            specific_area: 0
        show_state: true
        show_icon: true
        icon_height: 25px
        icon: mdi:alarm-note-off
  - type: entities
    entities:
      - entity: binary_sensor.porta_ingresso
        secondary_info: last-updated
      - entity: binary_sensor.radar_living
        secondary_info: last-updated
      - entity: binary_sensor.radar_camera
        secondary_info: last-updated
      - entity: binary_sensor.radar_mansarda
        secondary_info: last-updated
      - entity: binary_sensor.radar_lavanderia
        secondary_info: last-updated
      - entity: binary_sensor.persiana_bagno
        secondary_info: last-updated
      - entity: binary_sensor.persiana_cucina
        secondary_info: last-updated
      - entity: binary_sensor.persiana_living
        secondary_info: last-updated
    state_color: true
    show_header_toggle: false
```

## Troubleshooting-FAQ
If you have any problems, make the following checks:
- [x] Exit the programming mode of the control unit, the serial line does not work in programming mode
- [x] Check that the cables are connected correctly
- [x] Check the 232 converter is properly powered
- [x] Try to reverse TX and RX
- [x] (Only for Kyo32G) Verify that the central unit has firmware **2.13**, if it isn't you've to update central unit firmware to this version.

### Diagnostics Service
For diagnostics you can enable additional software logs through this service.
If necessary, contact me with an extract of the logs so that I can help you better.

``` yaml
service: esphome.espkyogate_debug_command
data:
  serial_trace: 1
  log_trace: 1
  polling_kyo: 1
```
* **serial_trace** Enable or Disable Serial Log communication to central unit
* **log_trace** Enable or Disable Application Log 
* **polling_kyo** Enable or Disable continuative polling to central unit (default always Enable)

## License
GNU AGPLv3 © [Lorenzo De Luca][https://lorenzodeluca.dev]
