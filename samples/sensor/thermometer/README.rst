.. _thermometer-sample:

Thermometer sample
##################

Overview
********

This sample application periodically measures the ambient temperature
at 1Hz. The result is written to the console.

Wiring
*******

VDD pin should be connected to 2.3V to 5.5V
GND pin connected to 0V
VOUT pin connected to the ADC input pin.

.. _`MCP970X Sensor`: http://ww1.microchip.com/downloads/en/devicedoc/20001942g.pdf

An overlay is provided for the nrf52840dk_nrf52840 board with the
sensor connected to pin AIN7.

Building and Running
********************

To build for the nrf52840dk_nrf52840 board use:

.. zephyr-app-commands::
	:zephyr-app: samples/sensor/thermometer
	:board: nrf52840dk_nrf52840
	:goals: build flash
	:compact:


To build for other boards and ambient temperature sensors, enable the sensor
node that supports ``SENSOR_CHAN_AMBIENT_TEMP`` and use an overlay to create an
alias named ``ambient-temp0`` to link to the node.  See the overlay used for the
``nrf52840dk_nrf52840`` board within this sample:
``boards/nrf52840dk_nrf52840.overlay``

Sample Output
=============

.. code-block:: console

	*** Booting Zephyr OS build zephyr-v3.3.0-1205-g118f73e12a70 ***
	Thermometer Example (arm)
	Temperature device is 0x6150, name is mcp9700a
	Temperature is 24.0°C
	Temperature is 24.1°C
	Temperature is 24.2°C
	Temperature is 24.1°C
	Temperature is 24.0°C
	Temperature is 24.1°C
