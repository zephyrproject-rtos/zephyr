.. _logger_ble_backend:

Logging: BLE Backend
########################

Overview
********

Sample that demonstrates how to setup and use the BLE Logging backend. The
BLE Logger uses the NRF Connect SDK NUS service as UUID to make it compatible
with already existing apps to debug BLE connections over UART.


Requirements
************

* A board with BLE support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/subsys/logging/ble_backend` in the
Zephyr tree.

The BLE logger can be tested with the NRF Toolbox app or any similar app that can connect over
BLE and detect the NRF NUS UUID service.
