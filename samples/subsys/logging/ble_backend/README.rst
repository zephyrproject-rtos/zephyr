.. zephyr:code-sample:: logging-ble-backend
   :name: Bluetooth logging backend
   :relevant-api: log_api log_backend bt_gatt

   Send log messages over Bluetooth using the Bluetooth logging backend.

Overview
********

Sample that demonstrates how to setup and use the Bluetooth Logging backend. The
Bluetooth Logger uses the NRF Connect SDK NUS service as UUID to make it compatible
with already existing apps to debug Bluetooth connections over UART.

The notification size of the Bluetooth backend buffer is dependent on the
transmission size of the mtu set with :kconfig:option:`CONFIG_BT_L2CAP_TX_MTU`. Be sure
to change this configuration to increase the logger throughput over Bluetooth.

Requirements
************

* A board with Bluetooth LE support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/subsys/logging/ble_backend` in the
Zephyr tree.

The Bluetooth logger can be tested with the NRF Toolbox app or any similar app that can connect over
Bluetooth and detect the NRF NUS UUID service.
