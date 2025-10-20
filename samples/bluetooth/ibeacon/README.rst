.. zephyr:code-sample:: bluetooth_ibeacon
   :name: iBeacon
   :relevant-api: bluetooth

   Advertise an Apple iBeacon using GAP Broadcaster role.

Overview
********

This simple application demonstrates the GAP Broadcaster role
functionality by advertising an Apple iBeacon. The calibrated RSSI @ 1
meter distance can be set using an IBEACON_RSSI build variable
(e.g. IBEACON_RSSI=0xb8 for -72 dBm RSSI @ 1 meter), or by manually
editing the default value in the ``main.c`` file.

Because of the hard-coded values of iBeacon UUID, major, and minor,
the application is not suitable for production use, but is quite
convenient for quick demonstrations of iBeacon functionality.

Requirements
************

* A board with Bluetooth LE support, or
* QEMU with BlueZ running on the host

Building and Running
********************

See :zephyr:code-sample-category:`bluetooth` samples for details on how
to run the sample inside QEMU.

For other boards, build and flash the application as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/ibeacon
   :board: <board>
   :goals: flash
   :compact:

Refer to your :ref:`board's documentation <boards>` for alternative
flash instructions if your board doesn't support the ``flash`` target.
