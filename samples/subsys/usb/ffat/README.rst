.. zephyr:code-sample:: usb-ffat
   :name: USB FFAT
   :relevant-api: usbd_api usbd_msc_device

   Expose a FFAT disk over USB using the USB Mass Storage Class implementation.

Overview
********

This sample application demonstrates the FFAT disk implementation using the new
experimental USB device stack. The sample has two FFAT disks that emulate FAT16
and FAT32 file system formatted disks. There is no real disk or file system
behind them. The two disks emulate FAT formatted disks at runtime. When mounted
on the host side, you will find three files on the FFAT16 disk and two files on
the FFAT32 disk. You can read and write to the binary files; the text files are
read-only. Writing to a file means that for each block written to the disk, a
callback is invoked on the Zephyr side that can be used, for example, to update
firmware or load something into the device running the Zephyr RTOS.

Requirements
************

This project requires an experimental USB device driver (UDC API).

Building and Running
********************

This sample can be built for multiple boards, in this example we will build it
for the nRF52840DK board:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/ffat
   :board: nrf52840dk/nrf52840
   :goals: build flash
   :compact:
