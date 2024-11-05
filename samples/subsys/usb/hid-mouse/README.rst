.. zephyr:code-sample:: usb-hid-mouse
   :name: USB HID mouse
   :relevant-api: usbd_api usbd_hid_device input_interface

   Implement a basic HID mouse device.

Overview
********

This sample app demonstrates use of a USB Human Interface Device (HID) driver
by the Zephyr project. This very simple driver enumerates a board with a button
into a mouse that has a left mouse button and optionally (depending on
the number of buttons on the board) a right mouse button, X-axis movement,
and Y-axis movement.
If the USB peripheral driver supports remote wakeup feature, wakeup request
will be performed on every button click if the bus is in suspended state.
This sample can be found under :zephyr_file:`samples/subsys/usb/hid-mouse` in the
Zephyr project tree.

Requirements
************

This project requires an USB device driver and uses the :ref:`input` API.
There must be a :dtcompatible:`gpio-keys` group of buttons or keys defined at
the board level that can generate input events, otherwise the example will build
but not work as expected.

The key mapping in the sample is as follows:

- ``INPUT_KEY_0``: left button
- ``INPUT_KEY_1``: right button
- ``INPUT_KEY_2``: move the mouse along the x-axis
- ``INPUT_KEY_3``: move the mouse along the y-axis

An LED must also be configured via the ``led0`` devicetree alias. You may also
see this error if you try to build this sample for an unsupported board:

.. code-block:: none

   Unsupported board: led0 devicetree alias is not defined

Building and Running
********************

This sample can be built for multiple boards, in this example we will build it
for the nucleo_f070rb board:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/hid-mouse
   :board: nucleo_f070rb
   :goals: build
   :compact:

After you have built and flashed the sample app image to your board, plug the
board into a host device, for example, a PC running Linux.
The board will be detected as shown by the Linux dmesg command:

.. code-block:: console

    dmesg | tail -10
    usb 2-2: new full-speed USB device number 2 using at91_ohci
    usb 2-2: New USB device found, idVendor=2fe3, idProduct=0007, bcdDevice= 2.03
    usb 2-2: New USB device strings: Mfr=1, Product=2, SerialNumber=3
    usb 2-2: Product: Zephyr HID mouse sample
    usb 2-2: Manufacturer: ZEPHYR
    usb 2-2: SerialNumber: 86FE679A598AC47A
    input: ZEPHYR Zephyr HID mouse sample as /devices/soc0/ahb/600000.ohci/usb2/2-2/2-2:1.0/0003:2FE3:0100.0001/input/input0
    hid-generic 0003:2FE3:0100.0001: input: USB HID v1.10 Mouse [ZEPHYR Zephyr HID mouse sample] on usb-at91-2/input0

You can also monitor mouse events by using the standard Linux ``evtest`` command
(see the `Ubuntu evtest man page`_ for more information about this tool):

.. _Ubuntu evtest man page:
   http://manpages.ubuntu.com/manpages/trusty/man1/evtest.1.html

.. code-block:: console

    sudo evtest /dev/input/event0
    Input driver version is 1.0.1
    Input device ID: bus 0x3 vendor 0x2fe3 product 0x7 version 0x110
    Input device name: "ZEPHYR Zephyr HID mouse sample"
    Supported events:
      Event type 0 (EV_SYN)
      Event type 1 (EV_KEY)
        Event code 272 (BTN_LEFT)
        Event code 273 (BTN_RIGHT)
        Event code 274 (BTN_MIDDLE)
      Event type 2 (EV_REL)
        Event code 0 (REL_X)
        Event code 1 (REL_Y)
        Event code 8 (REL_WHEEL)
      Event type 4 (EV_MSC)
        Event code 4 (MSC_SCAN)
    Properties:
    Testing ... (interrupt to exit)

When you press the button on your board, it will act as if the left
mouse button was pressed, and this information will be displayed
by ``evtest``:

.. code-block:: console

    Event: time 1167609663.618515, type 4 (EV_MSC), code 4 (MSC_SCAN), value 90001
    Event: time 1167609663.618515, type 1 (EV_KEY), code 272 (BTN_LEFT), value 1
    Event: time 1167609663.618515, -------------- SYN_REPORT ------------
    Event: time 1167609663.730510, type 4 (EV_MSC), code 4 (MSC_SCAN), value 90001
    Event: time 1167609663.730510, type 1 (EV_KEY), code 272 (BTN_LEFT), value 0
    Event: time 1167609663.730510, -------------- SYN_REPORT ------------

If your board has more than one button, they will act as right mouse button,
X-axis movement, and Y-axis movement.
