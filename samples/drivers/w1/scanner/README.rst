.. zephyr:code-sample:: w1-scanner
   :name: 1-Wire scanner
   :relevant-api: w1_interface

   Scan for 1-Wire devices and print their family ID and serial number.

Overview
********

This sample demonstrates how to scan for 1-Wire devices. It runs the 1-Wire
scan routine once and prints the family id as well as serial number of the
devices found.

Building and Running
********************

Set DTC_OVERLAY_FILE to "ds2484.overlay" or "w1_serial.overlay" in order to
enable and configure the drivers.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/w1/scanner
   :board: nrf52840dk/nrf52840
   :gen-args: -DDTC_OVERLAY_FILE=w1_serial.overlay
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

    [00:00:00.392,272] <inf> main: Device found; family: 0x3a, serial: 0x3af9985800000036
    [00:00:00.524,169] <inf> main: Device found; family: 0x3a, serial: 0x3a6ea2580000003b
    [00:00:00.656,097] <inf> main: Device found; family: 0x28, serial: 0x2856fb470d000022
    [00:00:00.656,097] <inf> main: Number of devices found on bus: 3
