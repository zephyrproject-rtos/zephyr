.. zephyr:code-sample:: shell-bluetooth
   :name: Shell over Bluetooth
   :relevant-api: shell_api bt_gatt

   Use Shell over the Bluetooth Backend.

Overview
********

This example provides access to shell over Bluetooth, exposed over GATT, using
Nordic UART Service UUID.

Requirements
************

A board with Bluetooth capabilities, a user-device with Bluetooth capabilities
to interact with the board and a user application to manage connection and
interaction with the board, compatible with the GATT NUS Service, e.g: Python
App Bleak, uart_service.py example.

Building
********

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/shell/bluetooth
   :board: nrf52840dk_nrf52840
   :goals: build
   :compact:

Running
*******

Once the board has booted, it will start advertising with the NUS UUID, as
well as the device name. The user is then able to connect and interact using
Shell over Bluetooth.

.. _`Bleak`: https://github.com/hbldh/bleak
