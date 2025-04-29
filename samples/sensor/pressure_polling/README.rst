.. zephyr:code-sample:: pressure_polling
   :name: Barometric pressure and temperature sensor polling example
   :relevant-api: sensor_interface

   Get barometric pressure and temperature data from a sensor.

Overview
********

This sample application periodically reads the sensor
temperature and pressure, displaying the
values on the console along with a timestamp since startup.
It also displays the estimated altitude if floating point is supported.

Wiring
*******

This sample uses an external breakout for the sensor.  A devicetree
overlay must be provided to identify the I2C/SPI used to control the sensor.

Building and Running
********************

This sample supports pressure sensor devices. Device needs
to be aliased as ``pressure-sensor``. For example:

.. code-block:: devicetree

	/ {
		aliases {
			pressure-sensor = &icp201xx;
		};
	};

Make sure the aliases are in devicetree, then build and run with:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/pressure_polling
   :board: nrf52dk/nrf52832
   :goals: build flash

Sample Output
=============

.. code-block:: console

## Default configuration

   Found device "icp101xx@63", getting sensor data
   [00:00:00.266,479] <inf> PRESSURE_POLLING: Starting pressure and altitude polling sample.
   [00:00:00.273,803] <inf> PRESSURE_POLLING: temp 25.49 Cel, pressure 96.271438 kPa, altitude 447.208465 m
   [00:00:00.280,914] <inf> PRESSURE_POLLING: temp 25.50 Cel, pressure 96.271331 kPa, altitude 447.234161 m
   [00:00:00.288,024] <inf> PRESSURE_POLLING: temp 25.49 Cel, pressure 96.266685 kPa, altitude 447.636077 m
   [00:00:00.295,135] <inf> PRESSURE_POLLING: temp 25.50 Cel, pressure 96.267951 kPa, altitude 447.537078 m
   [00:00:00.302,246] <inf> PRESSURE_POLLING: temp 25.51 Cel, pressure 96.268577 kPa, altitude 447.488281 m
   [00:00:00.309,356] <inf> PRESSURE_POLLING: temp 25.50 Cel, pressure 96.269340 kPa, altitude 447.414978 m
   [00:00:00.316,467] <inf> PRESSURE_POLLING: temp 25.50 Cel, pressure 96.268562 kPa, altitude 447.473663 m
   [00:00:00.323,547] <inf> PRESSURE_POLLING: temp 25.50 Cel, pressure 96.267341 kPa, altitude 447.596496 m

   <repeats endlessly>
