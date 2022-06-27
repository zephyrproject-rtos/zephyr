.. _wsen-itds:

WSEN-ITDS: 3-axis Acceleration Sensor
#####################################

Overview
********

This sample uses the Zephyr :ref:`sensor_api` API driver to periodically
read accelerations and temperature from the Würth Elektronik WSEN-ITDS
3-axis acceleration sensor and displays it on the console.

By default, samples are read in polling mode. If desired, the data-ready
interrupt of the sensor can be used to trigger reading of samples.

Additionally, the sensor's embedded functions (single/double tap,
free-fall and wake-up) can be tested by enabling the corresponding
triggers.

Requirements
************

This sample requires a WSEN-ITDS sensor connected via the I2C or SPI interface.

References
**********

- WSEN-ITDS: https://www.we-online.com/catalog/en/WSEN-ITDS

Building and Running
********************

This sample can be configured to support WSEN-ITDS sensors connected via
either I2C or SPI. Configuration is done via the :ref:`devicetree <dt-guide>`.
The devicetree must have an enabled node with ``compatible = "we,wsen-itds";``.
See :dtcompatible:`we,wsen-itds` for the devicetree binding.

By default, the sample reads from the sensor and outputs sensor data to the
console at regular intervals. If you want to test the sensor's trigger mode,
specify the trigger configuration in the prj.conf file and connect the
interrupt output from the sensor to your board.

The data-ready interrupt functionality can be tested by setting
``CONFIG_ITDS_TRIGGER_GLOBAL_THREAD=y`` or ``CONFIG_ITDS_TRIGGER_OWN_THREAD=y``
without enabling any additional triggers. When enabling one of the additional
triggers, output of sensor samples on the console is disabled. Instead, log
messages are printed when the selected events are triggered.

Additional triggers can be enabled by setting ``CONFIG_ITDS_TAP=y``,
``CONFIG_ITDS_FREEFALL=y`` or ``CONFIG_ITDS_DELTA=y``.
Note that the minimum recommended output data rate for tap recognition is
400 Hz, which can be configured in the devicetree by setting ``odr = "400";``
and ``op-mode = "high-perf";``.

Also note that the LSB of the sensor's I2C slave address can be modified using
the SAO pin. The I2C address used by the sensor (0x18 or 0x19) needs to be
specified in the devicetree. The WSEN-ITDS evaluation board uses 0x19 as its
default address.

.. zephyr-app-commands::
   :app: samples/sensor/wsen_itds/
   :goals: build flash

Sample Output
=============

.. code-block:: console

  [00:00:00.264,007] <inf> MAIN: ITDS device initialized.
  [00:00:00.265,655] <inf> MAIN: Sample #1
  [00:00:00.265,686] <inf> MAIN: Acceleration X: -2.89 m/s2
  [00:00:00.265,716] <inf> MAIN: Acceleration Y: 5.69 m/s2
  [00:00:00.265,747] <inf> MAIN: Acceleration Z: 4.51 m/s2
  [00:00:00.265,777] <inf> MAIN: Temperature: 29.8 C
  [00:00:02.267,517] <inf> MAIN: Sample #2
  [00:00:02.267,547] <inf> MAIN: Acceleration X: -1.39 m/s2
  [00:00:02.267,578] <inf> MAIN: Acceleration Y: 0.73 m/s2
  [00:00:02.267,608] <inf> MAIN: Acceleration Z: 9.59 m/s2
  [00:00:02.267,639] <inf> MAIN: Temperature: 29.6 C

   <repeats endlessly every 2 seconds>
