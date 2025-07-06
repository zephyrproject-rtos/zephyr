.. zephyr:code-sample:: dht_polling
   :name: Generic digital humidity temperature sensor polling
   :relevant-api: sensor_interface

   Get temperature and humidity data from a DHT sensor (polling mode).

Overview
********

This sample application demonstrates how to use digital humidity temperature
sensors.

Building and Running
********************

This sample supports up to 10 humidity/temperature sensors. Each sensor needs to
be aliased as ``dhtN`` where ``N`` goes from ``0`` to ``9``. For example:

.. code-block:: devicetree

  / {
	aliases {
			dht0 = &hs300x;
		};
	};


Make sure the aliases are in devicetree.

It also requires a correct fixture setup when the sensor is present.
For the correct execution of that sample in twister, add into boards's
map-file next fixture settings::

      - fixture: fixture_i2c_hs300x


Then build and run with:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/dht_polling
   :board: <board to use>
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

       hs300x@44: temp is 25.31 °C humidity is 30.39 %RH
       hs300x@44: temp is 25.51 °C humidity is 30.44 %RH
       hs300x@44: temp is 25.51 °C humidity is 30.37 %RH
       hs300x@44: temp is 25.51 °C humidity is 30.39 %RH
       hs300x@44: temp is 25.31 °C humidity is 30.37 %RH
       hs300x@44: temp is 25.31 °C humidity is 30.35 %RH
       hs300x@44: temp is 25.51 °C humidity is 30.37 %RH
       hs300x@44: temp is 25.51 °C humidity is 30.37 %RH
       hs300x@44: temp is 25.51 °C humidity is 30.39 %RH
       hs300x@44: temp is 25.51 °C humidity is 30.44 %RH
       hs300x@44: temp is 25.51 °C humidity is 30.53 %RH
