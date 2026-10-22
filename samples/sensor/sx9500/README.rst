.. zephyr:code-sample:: sx9500
   :name: SX9500 SAR proximity sensor
   :relevant-api: sensor_interface

   Get proximity data from an SX9500 SAR proximity sensor.

Overview
********

This sample shows how to use the Zephyr :ref:`sensor` API driver for the
`Semtech SX9500`_ capacitive SAR (Sense and Respond) proximity sensor.

.. _Semtech SX9500:
   https://www.semtech.com/products/smart-sensing/touch-proximity-devices/sx9500

The sample can run either in polling mode or trigger mode depending on
configuration. In polling mode, the application periodically fetches
and prints the proximity value. In trigger mode, the application
configures a near/far interrupt and prints the proximity value whenever
a trigger event occurs.

Building and Running
********************

The sample can be configured to support SX9500 sensors connected via I2C.
Configuration is done via :ref:`devicetree <dt-guide>`. The devicetree must
have an enabled node with ``compatible = "semtech,sx9500";``. See
:dtcompatible:`semtech,sx9500` for the devicetree binding and see below for
examples and common configurations.

If the sensor is not built into your board, start by wiring the sensor pins
according to the connection diagram given in the `SX9500 datasheet`_.
Refer to **page 4** of the datasheet for the pin diagram.

.. _SX9500 datasheet:
   https://www.semtech.com/products/smart-sensing/touch-proximity-devices/sx9500

Sample Output
=============

The sample prints output to the serial console. SX9500 device driver messages may also be logged. Refer to your board's documentation for information on
connecting to its serial console.

Here is example output for the default application settings, assuming that only
one SX9500 sensor is connected to the standard I2C pins.

The value printed as ``device is 2000XXXX`` represents the address of the device
structure in memory. The actual value depends on the board, memory layout,
and Zephyr version and should not be treated as a fixed constant.

Polling mode output:

.. code-block:: none

   *** Booting Zephyr OS vX.X.X ***
   device is 2000XXXX, name is SX9500
   prox is 0
   prox is 1
   prox is 1
   prox is 0

Trigger mode output:

.. code-block:: none

   *** Booting Zephyr OS vX.X.X ***
   device is 2000XXXX, name is SX9500
   prox is 1
   prox is 0
   prox is 1
