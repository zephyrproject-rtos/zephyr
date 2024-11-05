.. zephyr:code-sample:: legacy-usb-mass
   :name: Legacy USB Mass Storage
   :relevant-api: _usb_device_core_api

   Expose board's RAM as a USB disk using USB Mass Storage driver.

Overview
********

This sample app demonstrates use of a USB Mass Storage driver by the Zephyr
project. This very simple driver enumerates a board with RAM disk into an USB
disk. This sample can be found under
:zephyr_file:`samples/subsys/usb/legacy/mass` in the Zephyr project tree.

.. note::
   This samples demonstrate deprecated :ref:`usb_device_stack`.

Requirements
************

This project requires a USB device driver, and either 96KiB of RAM or a FLASH device.

Building and Running
********************

The configurations selects RAM-based disk without any file system.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/legacy/mass
   :board: reel_board
   :goals: build
   :compact:
