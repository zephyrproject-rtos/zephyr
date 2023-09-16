.. _vbatt_sample:

Battery Voltage Measurement Sample
##################################

Overview
********

This sample demonstrates how one can use the configuration of the Zephyr ADC infrastructure to
measure the voltage of the device power supply using a voltage-divider. The sample assumes a
regulator that provides a fixed voltage to IO38 of the esp32 behind a voltage divider. In this
sample the voltage divider consists of 300k and 100k Ohms resistors. It will then print the measured
battery voltage to the console. If you take care to not power the device thru the usb port, you can
see how the battery voltage decreases over time, Thus indicating the current charging level.

Application Details
===================

The application initializes battery measurement on startup, then loops
displaying the battery status every five seconds.

Requirements
************

A charging regulator (e.g. MAX777) connected to a voltage divider to the


Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/vbatt
   :host-os: unix
   :board: esp32_devkitc_wrover
   :goals: build flash
   :compact:

To build for another board, change "qemu_x86" above to that board's name.

Sample Output
=============

.. code-block:: console

    bat=1.85V
    bat=1.80V
    bat=1.85V
    bat=1.80V
