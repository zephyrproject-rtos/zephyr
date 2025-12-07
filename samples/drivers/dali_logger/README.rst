.. zephyr:code-sample:: dali-logger
   :name: Digital Addressable Lighting Interface (DALI) logger
   :relevant-api: dali

   Log activity on a DALI bus to UART.

Overview
********

This sample utilizes the :ref:`dali <dali_api>` driver API to log DALI frames to UART.

Building and Running
********************

The interface to the DALI bus is defined in the board's devicetree. For details see the dali_ sample.

.. note:: For proper operation a DALI specific physical interface is required.

Building and Running for Nordic nRF52840
============================================
The :zephyr_file:`samples/drivers/dali_logger/boards/nrf52840dk/nrf52840.overlay`
is specifically for the Mikroe-2672 DALI2 click development board
used as physical interface to the DALI bus. This board uses negative
logic for signal transmission (Tx Low <-> DALI Bus Idle).
The sample can be build and executed for the
:zephyr:board:`nrf52840` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dali
   :board: nrf52840dk/nrf52840
   :goals: build flash
   :compact:

Sample outout
=============

You should see log on DALI, when there's activity on the bus.
