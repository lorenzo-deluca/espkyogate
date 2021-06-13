
# ESPKyo32Gate for ESPhome

Serial Bridge for **Bentel Kyo32G Alarm Central**, based on **ESP8266** Board and **ESPHome** Open Source Firmware.
Thanks to @dario81 for initial porting to ESPHome 

## Hardware Connections
As board I used a **WeMos D1 Mini** (https://it.aliexpress.com/item/32651747570.html) but any board based on ESP8266 should be fine.

![ESP Connection](https://raw.githubusercontent.com/lorenzo-deluca/ESPKyo32Gate/master/images/ESP-Connections.JPG)

In order to connect to the serial port of the Kyo32 Unit I recommend a connector based on **MAX3232** chip, like this https://it.aliexpress.com/item/32722395554.html
This connector should be connected to the classic **TX/RX of the ESP board** and to the power supply (GND, 5V) on WeMos.
The WeMos can be powered with USB directly from the 12V output of the control unit by connecting any 12V->USB converter.
Which I recommend because in this way, even in case of power failure, the ESP is powered by the battery of the control unit.

![Bentel Kyo 32G ESP Connection](https://raw.githubusercontent.com/lorenzo-deluca/ESPKyo32Gate/master/images/BentelKYO32G-Connections.jpg)

## Firmware Preparation
The file `espkyogate_configuration.yaml` is already present in this repo, I suggest you start from this.
Set your WiFi ssid and password in `wifi` section.
Set `uart` settings in base depending on the board you use, example file is for Wemos D1 mini.
Finally edit `binary_sensors` you want to see on your Home Assistant as configured in the example file.

## Build and Upload Firmware
'python3 -m esphome espkyogate_configuration.yaml compile'
'python3 -m esphome espkyogate_configuration.yaml run'

## Check logs
'python3 -m esphome espkyogate_configuration.yaml logs'

## Usage

## License
GNU AGPLv3 © [Lorenzo De Luca][https://lorenzodeluca.dev]
