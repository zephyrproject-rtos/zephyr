.. zephyr:code-sample:: ble_peripheral_gatt_dynamic_service
   :name: Peripheral GATT Dynamic Service
   :relevant-api: bt_gatt bluetooth

   Demonstrate dynamic registration and unregistration of Bluetooth GATT services.

Overview
********

Sample application showing how to register and unregister GATT services dynamically at runtime. The application includes:

* A static service that is always available
* A dynamic service that toggles every 10 seconds
* Both services have read/write characteristics with custom UUIDs

The application continuously advertises and allows connections from central devices.

**Note: some BLE stacks/centrals may require reconnection to see GATT table updates.**

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/peripheral_gatt_dynamic_service` in the
Zephyr tree.

See :zephyr:code-sample-category:`bluetooth` samples for details.
