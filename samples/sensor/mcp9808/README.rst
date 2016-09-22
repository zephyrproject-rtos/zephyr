MCP9808 Temperature Sensor
##########################

Overview
========

Sample application that periodically reads temperature from the MCP9808 sensor.

The MCP9808 digital temperature sensor converts temperatures between -20°C and
+100°C to a digital word with ±0.5°C (max.) accuracy. It is I2C compatible and
supports up to 16 devices on the bus. We do not require pullup resistors on the
data or clock signals as they are already installed on the breakout board.

The MCP9808 is available in a discrete component form but it is much easier to
use it mounted on a breakout board. We used the Adafruit breakout board.

In this hookup we are only connecting one device to one of the supported boards.
It reads the temperature and displays it on the console.

Software
========

This sample uses the Zephyr sensor APIs and the provided driver for the MCP9808
sensor.

Pinouts
=======

The MCP9808 requires 2 wires for the I2C bus plus power and ground. The power
can be either 5V or 3.3V.

We connect the Data and clock wires to Analog ports A4 and A5 which is the I2C
pins on Arduino compatible boards.


Data Sheets
===========

http://www.microchip.com/wwwproducts/en/en556182

