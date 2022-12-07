.. _bluetooth-st_ble_sensor:

Bluetooth: ST BLE Sensor Demo
#############################

Overview
********

This application demonstrates BLE peripheral by exposing vendor-specific
GATT services. Currently only button notification and LED service are
implemented. Other BLE sensor services can easily be added.
See `BlueST protocol`_ document for details of how to add a new service.

Requirements
************

* `ST BLE Sensor Android app`_
* A board with BLE support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/st_ble_sensor` in the
Zephyr tree.

Open ST BLE Sensor app and click on "CONNECT TO A DEVICE" button to scan BLE devices.
To connect click on the device shown in the Device List.
After connected, tap LED image on Android to test LED service.
Push SW0 button on embedded device to test button service.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.

.. _ST BLE Sensor Android app:
    https://play.google.com/store/apps/details?id=com.st.bluems

.. _BlueST protocol:
    https://www.st.com/resource/en/user_manual/dm00550659.pdf
