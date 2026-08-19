.. zephyr:code-sample:: ble_peripheral_ans
   :name: Peripheral ANS
   :relevant-api: bluetooth

   Send notification using Alert Notification Service (ANS).

Overview
********

This sample demonstrates the usage of ANS by acting as a peripheral periodically sending
notifications to the connected remote device.

Requirements
************

* A board with Bluetooth LE support
* Smartphone with Bluetooth LE app (ADI Attach, nRF Connect, etc.) or dedicated Bluetooth LE sniffer

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/peripheral_ans
   :board: <board>
   :goals: build flash
   :compact:

After flashing, use a Bluetooth scanner app (e.g. nRF Connect) to connect to the device.
Enable notifications on the New Alert and Unread Alert Status characteristics, then write
to the Alert Notification Control Point to enable the desired alert categories. The sample
periodically sends Simple Alert and High Priority Alert notifications.
