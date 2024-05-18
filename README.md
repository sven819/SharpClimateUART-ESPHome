# SharpClimateUART-ESPHome
ESPHome Component for Sharp HVAC UART Protocol

## Supported Devices
* Sharp (unknown)
* Bosch (Climate 6000i (tested), 6100i, 8100i, 9100i)
* Buderus (Logacool AC166i (tested), AC186i, AC196i, AC176i.2, AC186.2)
* IVT (Aero 600, 800, 900)

If you use an untested device, please open an issue with the initial log (contains "Sharp INIT Data") and the model name.

## Usage

### Hardware
Tested with ESP12S Modul with 5v Levelconverter

#### Connect HVAC 
Parts:
- Crimp contacts:   SPHD-001T-P
- Connector:        PAP-08V-S
 

![CN13a](https://github.com/sven819/SharpClimateUART-ESPHome/blob/main/docs/cn13.png?raw=true)

Pinout HVAC <-> ESP: 
 * Black: GND <-> GND
 * White: RX <-> TX (5v Logic)
 * Green: TX <-> RX (5v Logic)
 * Red:  5V <-> VCC


### Software

To use this component in your ESPHome configuration, follow the example below:

#### Example configuration

```yaml
external_components:
  - source: component
    refresh: 0s

esphome:
  name: klima_wohnzimmer

esp8266:
  board: esp01_1m  

api:
ota:
web_server:
  port: 80

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

logger:
  baud_rate: 0 # disable serial logging if you're using the standard TX/RX pins for your serial peripheral
  level: DEBUG

uart:
  tx_pin: 1         # hardware dependent
  rx_pin: 3         # hardware dependent
  baud_rate: 9600
  parity: EVEN

button:
  - platform: restart
    name: "Living Room Restart"

climate:
  - platform: sharp_ac       # adjust to match your AC unit!
    name: "Living Room AC"
```

###  Adding this Component
Add the external_components entry to your ESPHome configuration file, pointing to the repository of this component.
Configure the uart section with the correct tx_pin and rx_pin for your hardware.
Set up the climate platform to sharp_ac and name it appropriately.

This project is provided "as is" without any warranty of any kind, express or implied. By using this project, you acknowledge that you do so at your own risk. The authors are not responsible for any damages or issues that may arise from using this software. Use it at your own discretion.

## Disclaimer
This repository is not affiliated with, endorsed by, or in any way connected to Sharp, Bosch, Buderus, or IVT. All product and company names are trademarks™ or registered® trademarks of their respective holders. Use of them does not imply any affiliation with or endorsement by them.
