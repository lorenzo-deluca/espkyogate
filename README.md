
# ESPKyo32Gate

Serial Bridge for **Bentel Kyo32G Alarm Central**, based on **ESP8266** Board and **MQTT Protocol**.

It was initially designed to work with the **Domoticz** open source Home Automation system.
Later, since I migrated my system to **Home Assistant** with the script below you can emulate the behavior of Domoticz with **manually configured mqtt entities** both on HA.

## Firmware Preparation
Set your SSID and MQTT in **ESPKyoDef.h** file.

    #define mqtt_server "enter mqtt server"
    #define mqtt_user ""
    #define mqtt_password ""
    
	#define wifi_ssid "enter wifi SSID"
	#define wifi_password "enter wifi password"
You have to flash this firmware on ESP8266 Board using Arduino software.

## Hardware Connections
As board I used a **WeMos D1 Mini** (https://it.aliexpress.com/item/32651747570.html) but any board based on ESP8266 should be fine.

![ESP Connection](https://raw.githubusercontent.com/lorenzo-deluca/ESPKyo32Gate/master/images/ESP-Connections.JPG)

In order to connect to the serial port of the Kyo32 Unit I recommend a connector based on **MAX3232** chip, like this https://it.aliexpress.com/item/32722395554.html
This connector should be connected to the classic **TX/RX of the ESP board** and to the power supply (GND, 5V) on WeMos.
The WeMos can be powered with USB directly from the 12V output of the control unit by connecting any 12V->USB converter.
Which I recommend because in this way, even in case of power failure, the ESP is powered by the battery of the control unit.

![Bentel Kyo 32G ESP Connection](https://raw.githubusercontent.com/lorenzo-deluca/ESPKyo32Gate/master/images/BentelKYO32G-Connections.jpg)

## Usage
When the card connects to WiFi and to the mqtt broker it publishes in the topic `ESPKyoGate/out` the message 
> "ESP Connesso".

After receiving this message you can continue with the configuration of **Zones** and **Areas**.
After these configurazion must be **enabled the dialogue** with the central unit and **saved the settings**.

All commands must be sent to topic `ESPKyoGate/in`, the result of the command will be sent to the topic `ESPKyoGate/out`

### Configure Zone
Then the **Zones** must be configured, starting from zone 1, for each zone an Idx must be specified, if you use Domoticz it will be the Id of the device that identifies the zone (pir, door, window, etc. ..), otherwise, even if you use Home Assistant you must assign a unique Idx for each zone.

For example this is a message to configure Zone 1, with Idx 21.

    conf zone 1 1 1 21

![MQTT Configure Zone](https://raw.githubusercontent.com/lorenzo-deluca/ESPKyo32Gate/master/images/configure-zone.png)

### Enable Updates and Unit Polling
After to have configured the Zones it must be activated the state update via MQTT  and enable the serial communication to the Central Unit Kyo.

First you have to activate the sending of zone status via MQTT to Domoticz (or HA if you configure the entities as described below):
> conf param DomoticzUpdate 1

After that you have to enable the serial communication to the Central Unit Kyo:
> start

Finally, the settings must be saved, otherwise all settings made so far will not be retained when the ESP card is restarted.
> save

## MQTT 
When the board connects to your MQTT Broker you can control it through these topics.

|Topic|Function|
|--|--|
|`ESPKyoGate/in` | Send Command to Board|
|`ESPKyoGate/out` | Firmware Logs and Command Outputs |
  
### MQTT Commands  

Publish **Command** in `ESPKyoGate/in` Topic

|Command|Function|
|--|--|
|`?` | Show avaiable commands in `ESPKyoGate/out`
|`conf zone <ZoneNumber> <Enabled> <ActiveState> <Idx> ` | Configure Zone
|`show zone <ZoneNumber>` | Show selected zone in `ESPKyoGate/out`
|`conf param DomoticzUpdate` | 
|`start` | Start polling Kyo32
|`stop` | Stop polling Kyo32
|`save` | Save current Configuration (Zone, Area)
|`restart` | Restart board without save
|`trace` | Enable serial trace logs in `ESPKyoGate/out`
|`notrace` | Disable serial trace logs
|`reset` | Erase all settings and restart!

### Config.yaml file for Home Assistant
You have to change `<RadarName>` / `<DoorName>` and `<AreaID>`
```yaml
binary_sensor:
  - platform: mqtt
    unique_id: "<RadarName>"
    name: "<RadarName>"
    state_topic: "domoticz/in"
    device_class: motion
    off_delay: 30
    value_template: " {% if value_json.idx == <AreaID> and value_json.switchcmd == 'On' %}
          {{'ON'}}
        {% endif %}"

 - platform: mqtt
    unique_id: <DoorName>
    name: "<DoorName>"
    state_topic: "domoticz/in"
    device_class: door
    value_template: " {% if value_json.idx == <AreaID> and value_json.switchcmd == 'On' %}
          {{'ON'}}
        {% elif value_json.idx == **<AreaID>** and value_json.switchcmd == 'Off' %}
          {{'OFF'}}
        {% endif %}"
```
