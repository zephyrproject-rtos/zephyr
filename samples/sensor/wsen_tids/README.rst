.. _wsen-tids:

WSEN-TIDS: Temperature Sensor
#############################

Overview
********

This sample uses the Zephyr :ref:`sensor_api` API driver to periodically
read the temperature from the Würth Elektronik WSEN-TIDS temperature
sensor and displays it on the console.

Additionally, the TIDS trigger function can be enabled. If enabled,
a message is printed when the temperature threshold interrupt has
been triggered.

Requirements
************

This sample requires a WSEN-TIDS sensor connected via the I2C interface.

References
**********

- WSEN-TIDS: https://www.we-online.com/catalog/en/WSEN-TIDS

Building and Running
********************

This sample supports WSEN-TIDS sensors connected via I2C. Configuration is
done via the :ref:`devicetree <dt-guide>`. The devicetree must have an
enabled node with ``compatible = "we,wsen-tids";``. See
:dtcompatible:`we,wsen-tids` for the devicetree binding.

The sample reads from the sensor and outputs sensor data to the console at
regular intervals. If you want to test the sensor's threshold trigger mode,
specify the trigger configuration in the prj.conf file and connect the
interrupt output from the sensor to your board.

Note that the sensor's interrupt pin is an open drain output. When used, the
pin must be connected to VDD via a pull-up resistor.

Also note that the sensor's I2C slave address can be modified using the SAO pin.
The I2C address used by the sensor (0x38 or 0x3F) needs to be specified in
the devicetree. The WSEN-TIDS evaluation board uses 0x38 as its default
address.

.. zephyr-app-commands::
   :app: samples/sensor/wsen_tids/
   :goals: build flash

Sample Output
=============

.. code-block:: console

   [00:00:00.371,887] <inf> MAIN: TIDS device initialized.
   [00:00:00.372,985] <inf> MAIN: Sample #1
   [00:00:00.373,016] <inf> MAIN: Temperature: 25.5 C
   [00:00:02.374,145] <inf> MAIN: Sample #2
   [00:00:02.374,176] <inf> MAIN: Temperature: 25.5 C
   [00:00:04.375,335] <inf> MAIN: Sample #3
   [00:00:04.375,366] <inf> MAIN: Temperature: 25.6 C

   <repeats endlessly every 2 seconds>
