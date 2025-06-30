.. zephyr:code-sample:: BLE-Shell
   :name: BLE Shell
   :relevant-api: bluetooth

   BLE backend to Zephyr Shell

Overview
********

This example demonstrates how to use the BLE backend for the Zephyr Shell.
The BLE backend implements a notification and write characteristic. A
connected device should subscribe to the notification characteristic and
then issue ASCII formatted commands to the write characteristic.

Note: Just like a UART based Shell, the user must send the 'newline' character
to indicate the end of a string command. The bytecode for the newline character
is 0x0A. The shell will then process the command and return the result via the
notification characteristic.

Requirements
************

Your board must:

#. Be BLE enabled

Building and Running
********************

Build and flash the BLE Shell sample as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/shell/shell_ble
   :board: max32655fthr
   :goals: build flash
   :compact:
