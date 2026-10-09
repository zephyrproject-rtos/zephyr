.. zephyr:code-sample:: usb-xbox360-controller
   :name: USB Xbox 360 controller
   :relevant-api: usbd_api usbd_xbox360_controller_device

   Implement a basic Xbox 360 controller.

Overview
********

This sample runs a simple loop that toggles each controller digital and analog control,
one after the other.

Requirements
************

This project requires a device USB controller.

Building and Running
********************

This sample can be built for multiple boards, in this example we will build it
for the samd21_xpro board:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/xbox360_controller
   :board: samd21_xpro
   :goals: build
   :compact:

After you have built and flashed the sample app image to your board, plug the
board into a computer, for example, a PC running Linux.
The Xbox controller will be detected as shown by the Linux dmesg command:

.. code-block:: console

    dmesg
    ...
    usb 1-1: New USB device found, idVendor=045e, idProduct=028e, bcdDevice= 4.04
    usb 1-1: New USB device strings: Mfr=1, Product=2, SerialNumber=3
    usb 1-1: Product: Controller
    usb 1-1: Manufacturer: Microsoft Corporation
    usb 1-1: SerialNumber: 140430AB514D355934202020FF110E3F
    input: Microsoft X-Box 360 pad as /devices/pci0000:00/0000:00:08.1/0000:c4:00.3/usb1/1-1/1-1:1.0/input/input182

To show the controller actions, you can use jstest-gtk on Linux or the built-in Game Controller
control panel on Windows. The Game Controller settings on macOS seem to be very limited.

Another simple way to display the controller actions is to use https://hardwaretester.com/gamepad,
it works on all three platforms.
