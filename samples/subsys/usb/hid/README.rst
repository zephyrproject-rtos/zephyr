.. _usb_hid:

USB HID Sample Application
##########################

Overview
********

This sample app demonstrates use of a USB Human Interface Device (HID) driver
by the Zephyr project.  This very simple driver is enumerated as a raw HID
device. This sample can be found under :zephyr_file:`samples/subsys/usb/hid` in the
Zephyr project tree.

Requirements
************

This project requires a USB device driver.

Building and Running
********************

This sample can be built for multiple boards, in this example we will build it
for the reel_board board:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/hid
   :board: reel_board
   :goals: build
   :compact:

After you have built and flashed the sample app image to your board, plug the
board into a host device, for example, a PC running Linux.
The board will be detected as shown by the Linux journalctl command:

.. code-block:: console

    $ journalctl -k -n 10
    usb 7-1: New USB device found, idVendor=2fe3, idProduct=0100
    usb 7-1: New USB device strings: Mfr=1, Product=2, SerialNumber=3
    usb 7-1: Product: Zephyr HID sample
    usb 7-1: Manufacturer: ZEPHYR
    usb 7-1: SerialNumber: 0.01
    input: ZEPHYR Zephyr HID sample as /devices/platform/vhci_hcd.0/usb7/7-1/7-1:1.0/0003:2FE3:0100.0046/input/input81
    hid-generic 0003:2FE3:0100.0046: input,hidraw0: USB HID v1.10 Device [ZEPHYR Zephyr HID sample] on usb-vhci_hcd.0-1/input0

You can monitor report sending using standard Linux ``usbhid-dump`` command.

.. code-block:: console

    $ sudo usbhid-dump -d 2fe3:0100 -es
    Starting dumping interrupt transfer stream
    with 1 minute timeout.

    007:012:000:STREAM             1537362690.341208
     01 02
