.. _analog_input:

Analog Input
############

Overview
********

The analog_input sample blinks the LED with control of the period
by the voltage of the input pin.
Inputting high voltage to blink the LED slowly.
Blink the LED fast on input voltage is low.
When the input is 0V, LED light.

Building and Running
********************

Build and flash analog_input sample as follows,

```sh
$> west build -p -b arduino_nano_33_ble sample/analog_input/

$> west flash --bossac=/home/$USER/.arduino15/packages/arduino/tools/bossac/1.9.1-arduino2/bossac
```
