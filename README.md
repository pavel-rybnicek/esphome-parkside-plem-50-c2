# esphome-parkside-plem-50-c2
Parkside PLEM 50 C2 laser range finder UART Integration for ESPHome

## Description
Parkside PLEM 50 C2 is a cheap (around €30) laser finder. I find this one very useful for monitoring water levels in water tanks and drainages.

The device consist of laser sensor module and main PCB + display module. Laser module is connected to main board with 4-conductor 1mm FFC cable - UART interface.  
For ESPHome integration we need the laser sensor module only.
Pinout of the FFC (left to right):
1. GND
2. VCC (3.3V)
3. Rx (connect to ESP Tx)
4. Tx (connect to ESP Rx)

## Usage
[See here for how to use external components](https://esphome.io/components/external_components.html).
See the example configuration file.

