.. zephyr:code-sample:: ble_peripheral_nus
   :name: Peripheral NUS
   :relevant-api: bluetooth

   Implement a simple echo server using the Nordic UART Service (NUS).

Overview
********

This sample demonstrates the usage of the NUS service (Nordic UART Service) as a serial
endpoint to exchange data. In this case, the sample assumes the data is UTF-8 encoded,
but it may be binary data. Once the user connects to the device and subscribes to the TX
characteristic, it will start receiving periodic notifications with "Hello World!\n".

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/peripheral_nus
   :board: <board>
   :goals: build flash
   :compact:

After flashing, use a Bluetooth scanner app (e.g. nRF Connect) to connect to the device
and subscribe to the NUS TX characteristic. The device will send periodic
``Hello World!`` notifications.
