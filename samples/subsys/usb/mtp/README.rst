.. zephyr:code-sample:: usb-mtp
   :name: USB MTP
   :relevant-api: usbd_api _usb_device_core_api file_system_api

   Expose the selected storage partition(s) as USB Media device using Media Transfer Protocol driver.

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

- USB device driver support.
- Storage media with Littlefs/FAT filesystem support.
- Partitions must be mounted before connecting the board to Host.

Building and Running
********************

This sample can be built for multiple boards which can support at least one storage parition,
in this example we will build it for the stm32f769i_disco board:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/mtp
   :board: stm32f769i_disco
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

The storage contents will persist across device reboots as long as the
filesystem is properly unmounted before disconnecting or resetting the device.

.. note::

   Not All MTP Features are implemented, for example you can't move/copy
   files from within the device, it must be done via Host.

Troubleshooting
===============

If the device is not recognized properly:

1. Ensure the storage medium is properly initialized
2. Check that the filesystem is mounted correctly
3. Verify USB configuration is correct for your board
4. Enable DEBUG logs on USB MTP driver to get more information

For debugging purposes, you can enable USB debug logs by setting the following
in your project's configuration:

.. code-block:: none

   CONFIG_USBD_LOG_LEVEL_DBG=y
   CONFIG_USBD_MTP_LOG_LEVEL_DBG=y
