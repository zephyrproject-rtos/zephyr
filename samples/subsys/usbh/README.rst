.. usb-host-sample:

USB Host Sample
###############

Overview
********

A simple sample to use an USB host controller to write and read a block
on a mass storage device.

Requirements
************

This sample has been tested with with the atsamr21_xpro and an USB hub with
it's own power supply

Building and Running
********************

This sample can be found under
:zephyr_file:`samples/subsys/usbh

The following commands build and flash the USB host sample.

.. zephyr-app-commands::
        :zephyr-app: samples/subsys/usbh
        :board: atsamr21_xpro
        :goals: build flash
        :compact:

Sample Output
=============

.. code-block:: console

*** Booting Zephyr OS build zephyr-v2.4.0-646-g2837ac4409a1  ***
[00:00:00.005,000] <inf> main: init return 0

[00:00:00.005,000] <inf> main: hc_add return 0

[00:00:00.005,000] <dbg> usbh_core.usbh_hc_start: start host controller
[00:00:00.005,000] <dbg> usbh_core.usbh_dev_conn: device connected
[00:00:00.005,000] <dbg> usbh_core.usbh_dev_addr_set: set device address
[00:00:00.005,000] <dbg> usbh_core.usbh_dev_conn: Port 0: Device Address: 11.

[00:00:00.006,000] <inf> hub: hub if
Product: USB 2.0 Hub [MTT]
[00:00:00.655,000] <dbg> usbh_core.usbh_dev_conn: device connected
[00:00:00.656,000] <dbg> usbh_core.usbh_dev_addr_set: set device address
[00:00:00.659,000] <dbg> usbh_core.usbh_dev_conn: Port 1: Device Address: 10.

[00:00:00.664,000] <inf> hub: hub if
Product: USB 2.0 Hub
[00:00:02.068,000] <wrn> hub: Ext HUB (Addr# 10) connected

[00:00:02.575,000] <dbg> usbh_core.usbh_dev_conn: device connected
[00:00:02.576,000] <dbg> usbh_core.usbh_dev_addr_set: set device address
[00:00:02.579,000] <dbg> usbh_core.usbh_dev_conn: Port 1: Device Address: 9.

[00:00:02.584,000] <inf> hub: hub if
Manufacturer: SanDisk
Product: Ultra USB 3.0
[00:00:03.386,000] <wrn> hub: Ext HUB (Addr# 9) connected

[00:00:03.639,000] <dbg> usbh_core.usbh_dev_conn: device connected
[00:00:03.640,000] <dbg> usbh_core.usbh_dev_addr_set: set device address
[00:00:03.643,000] <dbg> usbh_core.usbh_dev_conn: Port 2: Device Address: 8.

[00:00:03.651,000] <inf> hub: hub if
[00:00:03.651,000] <inf> main: state 1

[00:00:03.651,000] <inf> main: MSC connected

[00:00:03.673,000] <inf> main: blocks 240254976 blockSize 512

[00:00:03.673,000] <inf> msc: Mass Storage device (LUN 0) is initializing ...

[00:00:03.737,000] <inf> main: msc initialized

[00:00:03.738,000] <inf> main: unit rdy 1

[00:00:03.877,000] <inf> main: written 512 bytes

[00:00:03.887,000] <inf> main: read 512 bytes

[00:00:03.887,000] <inf> main: write and read buffers are equal
