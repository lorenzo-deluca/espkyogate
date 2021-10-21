# ESPhome for BENTEL KYO32G
[![buy me a coffee](https://img.shields.io/badge/support-buymeacoffee-222222.svg?style=flat-square)](https://www.buymeacoffee.com/lorenzodeluca)

Serial Bridge for **Bentel Kyo32G Alarm Central**, based on **ESP8266** Board and **ESPHome** Open Source Firmware.
Thanks to @dario81 for initial porting to ESPHome.

If you like this project you can support me with :coffee: or simply put a :star: to this repository :blush:

<a href="https://www.buymeacoffee.com/lorenzodeluca" target="_blank">
  <img src="https://www.buymeacoffee.com/assets/img/custom_images/yellow_img.png" alt="Buy Me A Coffee" width="150px">
</a>

> **disclaimer** This software was developed by analyzing serial messages from/to central, it was not sponsored or officially supported by **Bentel**

If someone from **Bentel** would like to contribute or collaborate please contact me at [me@lorenzodeluca.dev](mailto:me@lorenzodeluca.dev?subject=[GitHub]ESPKyoGate)

## Hardware Connections
As board I used a **WeMos D1 Mini** (https://it.aliexpress.com/item/32651747570.html) but any board based on ESP8266 should be fine.

![ESP Wiring](https://raw.githubusercontent.com/lorenzo-deluca/espkyogate/master/images/wiring.png)

In order to connect to the serial port of the Kyo32 Unit I recommend a connector based on **MAX3232** chip, like this https://it.aliexpress.com/item/32722395554.html
This connector should be connected to the classic **TX/RX of the ESP board** and to the power supply (GND, 5V) on WeMos.

![Central Connections](https://raw.githubusercontent.com/lorenzo-deluca/espkyogate/master/images/BentelKYO32G-Connections.jpg)

The WeMos can be powered with USB directly from the 12V output of the control unit by connecting any 12V->USB converter.

Which I recommend because in this way, even in case of power failure, the ESP is powered by the battery of the control unit.

## Firmware Preparation
The file `espkyogate_configuration.yaml` is already present in this repo.
I suggest you start from this.

Set your WiFi ssid and password in `wifi` section.

Set `uart` settings in base depending on the board you use, example file is for Wemos D1 mini.
Finally edit `binary_sensors` you want to see on your Home Assistant as configured in the example file.

### Build and Upload Firmware

#### With ESPHome 
This way is the easiest, just copy the files from this repository to the esphome folder, edit the `espkyogate_configuration.yaml` file as above, upload and see if everything works from the logs.
You should see something similar.
![ESPHomeLogs](https://raw.githubusercontent.com/lorenzo-deluca/espkyogate/master/images/ESPHomeLogs.png)

#### From esphome command line
`python3 -m esphome espkyogate_configuration.yaml compile`
`python3 -m esphome espkyogate_configuration.yaml run`

##### Check logs
See logs with this command 
`python3 -m esphome espkyogate_configuration.yaml logs`

Output should be the same as above.

# Usage
If everything went well now you should find a new device in Home Assistant, called **espkyogate**.
Previously configured sensors will be automatically created and associated to the device.

![ESP Wiring](https://raw.githubusercontent.com/lorenzo-deluca/espkyogate/master/images/HomeAssistant-Lovelace.png)

## Avaiable Services
These methods will be available in the services

### Area Arm
``` yaml
service: esphome.espkyogate_arm_area
data:
  arm_type: 1 (total arm) - 2 (partially arm)
  area: <area_number>
  specific_area: 1 (arm <area_number> and disarm others) - 0 (arm only <area_number> without changing the others)
```

### Area Disarm
``` yaml
service: esphome.espkyogate_disarm_area
data:
  area: <area_number>
  specific_area: 1 (disarm all areas) - 0 (disarm only <area_number> without changing the others)
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

## License
GNU AGPLv3 © [Lorenzo De Luca][https://lorenzodeluca.dev]
