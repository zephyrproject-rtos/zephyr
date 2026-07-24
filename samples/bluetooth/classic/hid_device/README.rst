.. zephyr:code-sample:: bluetooth_hid_device
   :name: HID Device
   :relevant-api: bt_hid bluetooth

   Expose a Bluetooth Classic HID keyboard device.

Overview
********

This sample registers a Bluetooth Classic HID device with a boot keyboard report
descriptor. After a host connects and opens the HID interrupt channel, the device
periodically sends an ``a`` key press and release sequence.

Requirements
************

* A board with Bluetooth BR/EDR (Classic) support, or
* BlueZ on a Linux host for interoperability testing

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/classic/hid_device
   :board: sf32lb52_devkit_lcd/sf32lb525uc6
   :goals: build flash
   :compact:

After flashing, the device advertises as a discoverable HID keyboard. Pair and
connect from a host PC or phone, then observe the periodic key events in the
serial log.

See :zephyr:code-sample-category:`bluetooth_classic` for general Classic build
instructions.
