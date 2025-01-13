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

This sample can be found under :zephyr_file:`samples/bluetooth/st_ble_sensor` in the
Zephyr tree.

Open ST Bluetooth LE Sensor app and click on "CONNECT TO A DEVICE" button to scan Bluetooth LE devices.
To connect click on the device shown in the Device List.
After connected, tap LED image on Android to test LED service.
Push SW0 button on embedded device to test button service.

See :zephyr:code-sample-category:`bluetooth` samples for details.

.. _ST Bluetooth LE Sensor Android app:
    https://play.google.com/store/apps/details?id=com.st.bluems

.. _BlueST protocol:
    https://www.st.com/resource/en/user_manual/dm00550659.pdf
