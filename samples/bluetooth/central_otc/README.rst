.. zephyr:code-sample:: ble_central_otc
   :name: Central OTC
   :relevant-api: bluetooth

   Connect to a Bluetooth LE peripheral that supports the Object Transfer Service (OTS)

Overview
********

Similar to the :zephyr:code-sample:`ble_central` sample, except that this
application specifically looks for the OTS (Object Transfer) GATT Service.
And this sample is to select object sequentially, to read metadata, to write data,
to read data, and to calculate checksum of selected objects.

Requirements
************

* A board with Bluetooth LE support and 4 buttons.

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/central_otc
   :board: <board>
   :goals: build flash
   :compact:

After flashing, the sample scans for a peripheral advertising the Object
Transfer Service (OTS) and connects to it. Use the
:zephyr:code-sample:`ble_peripheral_ots` sample on a second board as the
peripheral. Once connected, use the four buttons to interact with objects:

* Button 1 (SW0): selects the first available object on the first press,
  advances to the next object on subsequent presses
* Button 2 (SW1): reads object metadata
* Button 3 (SW2): writes object data
* Button 4 (SW3): reads object data
