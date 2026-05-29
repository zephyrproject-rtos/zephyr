.. zephyr:code-sample:: bluetooth_bthome_sensor_template
   :name: BTHome sensor template
   :relevant-api: bluetooth

   Implement a BTHome sensor.

Overview
********

This code sample provides a template for implementing a `BTHome <https://bthome.io/>`_ sensor.

Requirements
************

* A board with Bluetooth LE support
* A BTHome compatible listener, for example `Home Assistant <https://www.home-assistant.io/>`_ with the BTHome integration running.

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/bthome_sensor_template
   :board: <board>
   :goals: build flash
   :compact:

When the sample is running, navigate to Devices & Services under settings in Home
Assistant. There you will be asked to configure the BTHome sensor if everything
went well.
