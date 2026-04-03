.. zephyr:code-sample:: fuel_gauge
   :name: Fuel Gauge

   Use fuel gauge API to access fuel gauge properties and get charge information.

Overview
********

This sample shows how to use the Zephyr :ref:`fuel_gauge_api` API driver for.

First, the sample tries to read all public fuel gauge properties to verify which are supported by
the used fuel gauge driver.
Second, the sample then reads the battery percentage and voltage proper periodically using the
``FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE`` and ``FUEL_GAUGE_VOLTAGE`` properties.

.. note::
   The sample does not set/write any properties to avoid misconfiguration the IC.

Building and Running
********************

The sample can be configured to support a fuel gauge.

Features
********
By using this fuel gauge you can get the following information:

  * Read all public fuel gauge properties
  * Battery charge status as percentage (periodically)
  * Battery voltage (periodically)

Sample output
*************

.. code-block:: console

   *** Booting Zephyr OS build f95fd665ad26 ***
   [00:00:00.116,000] <inf> app: Found device "lc709203f@0b"
   [00:00:00.116,000] <inf> app: Test-Read generic fuel gauge properties to verify which are supported
   [00:00:00.116,000] <inf> app: Info: not all properties are supported by all fuel gauges!
   [00:00:00.116,000] <inf> app: Property "FUEL_GAUGE_AVG_CURRENT" is not supported
   [00:00:00.116,000] <inf> app: Property "FUEL_GAUGE_CURRENT" is not supported
   [00:00:00.116,000] <inf> app: Property "FUEL_GAUGE_CHARGE_CUTOFF" is not supported
   [00:00:00.116,000] <inf> app: Property "FUEL_GAUGE_CYCLE_COUNT" is not supported
   [00:00:00.116,000] <inf> app: Property "FUEL_GAUGE_CONNECT_STATE" is not supported
   [00:00:00.116,000] <inf> app: Property "FUEL_GAUGE_FLAGS" is not supported
   [00:00:00.116,000] <inf> app: Property "FUEL_GAUGE_FULL_CHARGE_CAPACITY" is not supported
   [00:00:00.116,000] <inf> app: Property "FUEL_GAUGE_PRESENT_STATE" is not supported
   [00:00:00.116,000] <inf> app: Property "FUEL_GAUGE_REMAINING_CAPACITY" is not supported
   [00:00:00.116,000] <inf> app: Property "FUEL_GAUGE_RUNTIME_TO_EMPTY" is not supported
   [00:00:00.116,000] <inf> app: Property "FUEL_GAUGE_RUNTIME_TO_FULL" is not supported
   [00:00:00.116,000] <inf> app: Property "FUEL_GAUGE_SBS_MFR_ACCESS" is not supported
   [00:00:00.116,000] <inf> app: Property "FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE" is not supported
   [00:00:00.122,000] <inf> app: Property "FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE" is supported
   [00:00:00.122,000] <inf> app:   Relative state of charge: 100
   [00:00:00.122,000] <err> lc709203f: Thermistor not enabled
   [00:00:00.122,000] <inf> app: Property "FUEL_GAUGE_TEMPERATURE" is not supported
   [00:00:00.123,000] <inf> app: Property "FUEL_GAUGE_VOLTAGE" is supported
   [00:00:00.123,000] <inf> app:   Voltage: 4190000
   [00:00:00.124,000] <inf> app: Property "FUEL_GAUGE_SBS_MODE" is supported
   [00:00:00.124,000] <inf> app:   SBS mode: 1
   [00:00:00.124,000] <inf> app: Property "FUEL_GAUGE_CHARGE_CURRENT" is not supported
   [00:00:00.124,000] <inf> app: Property "FUEL_GAUGE_CHARGE_VOLTAGE" is not supported
   [00:00:00.124,000] <inf> app: Property "FUEL_GAUGE_STATUS" is not supported
   [00:00:00.124,000] <inf> app: Property "FUEL_GAUGE_DESIGN_CAPACITY" is supported
   [00:00:00.124,000] <inf> app:   Design capacity: 500
   [00:00:00.124,000] <inf> app: Property "FUEL_GAUGE_DESIGN_VOLTAGE" is not supported
   [00:00:00.124,000] <inf> app: Property "FUEL_GAUGE_SBS_ATRATE" is not supported
   [00:00:00.124,000] <inf> app: Property "FUEL_GAUGE_SBS_ATRATE_TIME_TO_FULL" is not supported
   [00:00:00.124,000] <inf> app: Property "FUEL_GAUGE_SBS_ATRATE_TIME_TO_EMPTY" is not supported
   [00:00:00.124,000] <inf> app: Property "FUEL_GAUGE_SBS_ATRATE_OK" is not supported
   [00:00:00.125,000] <inf> app: Property "FUEL_GAUGE_SBS_REMAINING_CAPACITY_ALARM" is not supported
   [00:00:00.125,000] <inf> app: Property "FUEL_GAUGE_SBS_REMAINING_TIME_ALARM" is not supported
   [00:00:00.125,000] <err> app: Error: cannot get property "FUEL_GAUGE_MANUFACTURER_NAME": -88
   [00:00:00.125,000] <err> app: Error: cannot get property "FUEL_GAUGE_DEVICE_NAME": -88
   [00:00:00.125,000] <err> app: Error: cannot get property "FUEL_GAUGE_DEVICE_CHEMISTRY": -88
   [00:00:00.125,000] <inf> app: Property "FUEL_GAUGE_CURRENT_DIRECTION" is supported
   [00:00:00.125,000] <inf> app:   Current direction: 0
   [00:00:00.126,000] <inf> app: Property "FUEL_GAUGE_STATE_OF_CHARGE_ALARM" is supported
   [00:00:00.126,000] <inf> app:   State of charge alarm: 8
   [00:00:00.127,000] <inf> app: Property "FUEL_GAUGE_LOW_VOLTAGE_ALARM" is supported
   [00:00:00.127,000] <inf> app:   Low voltage alarm: 0
   [00:00:00.127,000] <inf> app: Polling fuel gauge data 'FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE' & 'FUEL_GAUGE_VOLTAGE'
   [00:00:00.128,000] <inf> app: Fuel gauge data: Charge: 100%, Voltage: 4190mV
   [00:00:05.130,000] <inf> app: Fuel gauge data: Charge: 100%, Voltage: 4190mV
   [00:00:10.131,000] <inf> app: Fuel gauge data: Charge: 100%, Voltage: 4190mV
