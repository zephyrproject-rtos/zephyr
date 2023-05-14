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


Temperature Alert
=================

If the attached sensor supports alerts when the temperature drifts above or
below a threshold, the sample will enable the sensor's trigger functionality.
This will require the sensor's TRIGGER KConfig setting to be enabled. An
example of this setup is provided for the ``frdm_k22f`` board, using
``boards/frdm_k22f.conf``.

Sample Output
=============

.. code-block:: console

        *** Booting Zephyr OS build zephyr-v3.3.0-2354-gb4f4bd1f1c22 ***
        Thermometer Example (arm)
        Temperature device is 0x525c, name is tcn75a@48
        Set temperature lower limit to 25.5°C
        Set temperature upper limit to 26.5°C
        Enabled sensor threshold triggers
        Temperature is 25.0°C
        Temperature is 25.0°C
        Temperature is 25.0°C
        Temperature is 25.0°C
        Temperature is 25.5°C
        Temperature above threshold: 26.5°C
        Temperature is 26.5°C
