.. zephyr:code-sample:: usb-hid-keyboard
   :name: USB HID keyboard
   :relevant-api: usbd_api usbd_hid_device input_interface

   Implement a basic HID keyboard device.

Overview
********

This sample application demonstrates the HID keyboard implementation using the
new experimental USB device stack.

Requirements
************

This project requires an experimental USB device driver (UDC API) and uses the
:ref:`input` API. There must be a :dtcompatible:`gpio-keys` group of buttons
or keys defined at the board level that can generate input events.
At least one key is required and up to four can be used. The first three keys
are used for Num Lock, Caps Lock and Scroll Lock. The fourth key is used to
report HID keys 1, 2, 3 and the right Alt modifier at once.

The example can use up to three LEDs, configured via the devicetree alias such
as ``led0``, to indicate the state of the keyboard LEDs.

Building and Running
********************

This sample can be built for multiple boards, in this example we will build it
for the nRF52840DK board:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/hid-keyboard
   :board: nrf52840dk/nrf52840
   :goals: build flash
   :compact:
