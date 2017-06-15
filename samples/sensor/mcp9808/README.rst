.. _mcp9808-sample:

MCP9808 Temperature Sensor
##########################

Overview
********

Sample application that periodically reads temperature from the MCP9808 sensor.


Requirements
************

The MCP9808 digital temperature sensor converts temperatures between -20 |deg|
C and +100 |deg| C to a digital word with |plusminus| 0.5 |deg| C (max.)
accuracy. It is I2C compatible and
supports up to 16 devices on the bus. We do not require pullup resistors on the
data or clock signals as they are already installed on the breakout board.

The MCP9808 is available in a discrete component form but it is much easier to
use it mounted on a breakout board. We used the Adafruit breakout board.

- `MCP9808 Sensor`_

This sample uses the sensor APIs and the provided driver for the MCP9808 sensor.

Wiring
*******

The MCP9808 requires 2 wires for the I2C bus plus power and ground. The power
can be either 5V or 3.3V.

We connect the Data and clock wires to Analog ports A4 and A5 which is the I2C
pins on Arduino compatible boards.

In this hookup we are only connecting one device to one of the supported boards.
It reads the temperature and displays it on the console.


References
***********

- http://www.microchip.com/wwwproducts/en/en556182


.. _`MCP9808 Sensor`: https://www.adafruit.com/product/1782
