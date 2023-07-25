.. _lz4:

LZ4
###

Overview
********

A simple sample that can be used with any :ref:`supported board <boards>` and
compress & decompress the user data to the console.

Building and Running
********************

The sample can be built and executed on nrf52840dk_nrf52840 as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/compression/lz4
   :board: nrf52840dk_nrf52840
   :goals: build flash
   :compact:

To build for another board, change "nrf52840dk_nrf52840" above to that board's name.
