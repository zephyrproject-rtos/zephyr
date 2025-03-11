.. zephyr:code-sample:: usb-mtp
   :name: USB MTP
   :relevant-api: usbd_api _usb_device_core_api file_system_api

   Use USB MTP driver which implement a Media Transfer Protocol device.

Overview
********

This sample app demonstrates the use of USB Media Transfer Protocol (MTP)
driver provided by the Zephyr project. It allows the device to be mounted
as a media device on the host system, enabling file transfers between the
host and device.
This sample can be found under :zephyr_file:`samples/subsys/usb/mtp` in the
Zephyr project tree.

Requirements
************

This project requires:
- USB device driver support
- Storage media with Littlefs/FAT filesystem support

Building and Running
********************

Reel Board
===========

Build and flash the project:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/mtp
   :board: reel_board
   :goals: flash
   :compact:

Running
=======

Plug the board into a host device, for example, a PC running Linux.
The board will be detected as shown by the Linux dmesg command:

.. code-block:: console

   usb 9-1: new full-speed USB device number 112 using uhci_hcd
   usb 9-1: New USB device found, idVendor=8086, idProduct=f8a1
   usb 9-1: New USB device strings: Mfr=1, Product=2, SerialNumber=3
   usb 9-1: Product: Zephyr MTP
   usb 9-1: Manufacturer: ZEPHYR
   usb 9-1: SerialNumber: 0123456789AB

Once connected, the device should appear as a media device on your host system.
You can then:
- Browse the device storage
- Transfer files to/from the device
- Create/delete directories
- Manage files on the device

The storage contents will persist across device reboots as long as the
filesystem is properly unmounted before disconnecting or resetting the device.

Troubleshooting
==============

If the device is not recognized properly:

1. Ensure the storage medium is properly initialized
2. Check that the filesystem is mounted correctly
3. Verify USB configuration is correct for your board
4. Some systems may require additional MTP device support to be installed

For debugging purposes, you can enable USB debug logs by setting the following
in your project's configuration:

.. code-block:: none

   CONFIG_USB_DEVICE_LOG_LEVEL_DBG=y
   CONFIG_USB_MTP_LOG_LEVEL_DBG=y