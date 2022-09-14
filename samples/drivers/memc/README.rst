.. zephyr:code-sample:: memc
   :name: Memory controller (MEMC) driver

   Access memory-mapped external RAM

Overview
********

This sample can be used with any memory controller driver that
memory maps external RAM. It is intended to demonstrate
the ability to read and write from the memory mapped region.

Note that the chosen region (set by ``sram-ext`` dt alias node) should not
overlap with memory used for XIP or SRAM by the application, as the sample
would overwrite this data


Building and Running
********************

This application can be built and executed on an RT595 EVK as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/memc
   :host-os: unix
   :board: mimxrt595_evk/mimxrt595s/cm33
   :goals: run
   :compact:

To build for another board, change "mimxrt595_evk/mimxrt595s/cm33" above to that
board's name.

Sample Output
=============

.. code-block:: console

*** Booting Zephyr OS build zephyr-v3.2.0-2686-gd52d828c2bdc ***
Writing to memory region with base 0x38000000, size 0x800000

First 1KB of Data in memory:
\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=
00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f
10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f
20 21 22 23 24 25 26 27 28 29 2a 2b 2c 2d 2e 2f
....

Read data matches written data
