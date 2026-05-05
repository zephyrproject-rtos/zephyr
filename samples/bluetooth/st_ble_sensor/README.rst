.. zephyr:code-sample:: bluetooth_st_ble_sensor
   :name: ST Bluetooth LE Sensor Demo
   :relevant-api: bt_gatt bluetooth

   Export vendor-specific GATT services over Bluetooth.

Overview
********

This application demonstrates Bluetooth LE peripheral by exposing vendor-specific
GATT services. Currently only button notification and LED service are
implemented. Other Bluetooth LE sensor services can easily be added.
See `BlueST protocol`_ document for details of how to add a new service.

Requirements
************

* `ST Bluetooth LE Sensor Android app`_
* A board with Bluetooth LE support

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/st_ble_sensor
   :board: <board>
   :goals: build flash
   :compact:

Open the `ST Bluetooth LE Sensor Android app`_ and tap "CONNECT TO A DEVICE" to scan for
Bluetooth LE devices. Tap the device in the list to connect. Once connected, tap the LED
image in the app to test the LED service, or push the SW0 button on the board to test the
button notification service.

.. _ST Bluetooth LE Sensor Android app:
    https://play.google.com/store/apps/details?id=com.st.bluems

.. _BlueST protocol:
    https://www.st.com/resource/en/user_manual/dm00550659.pdf
