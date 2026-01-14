.. zephyr:code-sample:: max3010x_interr
   :name: Max3010x rwa value acquisition
   :relevant-api: sensor_interface

   Get reflectacne value of red, ir and green leds from Max3010x sensors (interrupt mode).

Overview
********

This sample application periodically reads the reflectance value from the
the various leds of the MAX17262 device that implements sensor_sample_fetch,
sensor_channel_get, using SENSOR_CHAN_RED and SENSOR_CHAN_IR.

Requirements
************

The MAX3010x is an ultra-low power Pulse Oximeter and Heart-Rate monitor
module.The Max3010x operates on a single 1.8V power supply and a separate 3.3V
power supply for the internal LEDs.Communication is through a standard
I2C-compatible interface. The module can be shut down through software with zero
standby current,allowing the power rails to remain powered at all times.

This sample requires a board which provides a configuration for Arduino
connectors and defines node aliases for the I2C interface and the mdoule configuration.
For more info about the node structure see
:zephyr_file:`samples/sensor/max3010x_interr/boards/nrf54l15dk_nrf54l15_cpuapp.overlay`

Building and Running
********************

This sample application uses an MAX17262 sensor connected to a board via I2C.
Connect the sensor pins according to the connection diagram given in the
max30102 datasheet.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/max3010x_interr
   :board: nrf54l15dk/nrf54l15_cpuapp
   :goals: build flash
   :compact:

Sample Output
=============
To check output of this sample , any serial console program can be used.
This example uses putty on the serial port ``/dev/ttyACM0``:

.. code-block:: console

        Interrupt!	Red:	732,	IR:	560
        Interrupt!	Red:	735,	IR:	558
        Interrupt!	Red:	729,	IR:	559
        Interrupt!	Red:	733,	IR:	559
        Interrupt!	Red:	736,	IR:	561

References
**********

.. target-notes::

.. _max30102 datasheet: https://www.analog.com/media/en/technical-documentation/data-sheets/max30102.pdf
