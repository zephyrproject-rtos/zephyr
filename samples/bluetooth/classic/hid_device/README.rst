.. zephyr:code-sample:: bluetooth_hid_device
   :name: HID Device
   :relevant-api: bt_hid_device bluetooth

   Use Classic Bluetooth HID Device (HIDP) keyboard functionality.

Overview
********

This sample demonstrates the Classic Bluetooth HID Device Profile using
Zephyr's BR/EDR HID Device APIs. The application registers as a HID keyboard,
exposes an SDP service record with a HID report descriptor, and becomes
discoverable so a HID host (for example a PC) can connect and receive input
reports.

Characters typed on the Zephyr console are converted to HID keyboard reports
and sent to the connected host.

Requirements
************

* A board with Bluetooth BR/EDR (Classic) support, or
* BlueZ running on the host

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/classic/hid_device
   :board: sf32lb52_devkit_lcd/sf32lb525uc6
   :goals: build flash
   :compact:

This sample has been validated on :zephyr:board:`sf32lb52_devkit_lcd`.

After flashing, the device becomes discoverable as ``Zephyr HID Keyboard``.
Pair and connect from a Classic Bluetooth HID host. Once the HID control and
interrupt channels are established, type characters on the serial console to
send key reports to the remote host.

See :zephyr:code-sample-category:`bluetooth` samples for details.
