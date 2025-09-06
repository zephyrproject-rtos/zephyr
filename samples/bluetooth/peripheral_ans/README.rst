.. zephyr:code-sample:: ble_peripheral_ans
   :name: Peripheral ANS
   :relevant-api: bluetooth

   Sample application for Alert Notification Service (ANS).

Overview
********

This sample demonstrates the usage of ANS by acting as a peripheral periodically sending
notifications to the connected central.

Requirements
************

* A board with Bluetooth LE support
* Smartphone with BLE app (ADI Attach, nRF Connect, etc.) or dedicated BLE sniffer

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/peripheral_ans` in the
Zephyr tree.

To start receiving alerts over the connection, refer to
`GATT Specification Supplement <https://btprodspecificationrefs.blob.core.windows.net/gatt-specification-supplement/GATT_Specification_Supplement.pdf>`_
section 3.12 for byte array to enable/disable notifications and control the service.

See :zephyr:code-sample-category:`bluetooth` samples for details.
