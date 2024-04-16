.. _bluetooth-bthome-sensor-template-sample:

Bluetooth: BTHome sensor template
#################################

Template for a `BTHome <https://bthome.io/>`_ sensor.

Requirements
************

* A board with BLE support
* A BTHome compatible listener, for example `Home Assistant <https://www.home-assistant.io/>`_ with the BTHome integration running.

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/bthome_sensor_template` in the Zephyr tree.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.

When the sample is running, navigate to Devices & Services under settings in Home
Assistant. There you will be asked to configure the BTHome sensor if everything
went well.
