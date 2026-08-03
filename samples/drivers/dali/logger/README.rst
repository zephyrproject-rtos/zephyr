.. zephyr:code-sample:: dali_logger
   :name: Digital Addressable Lighting Interface (DALI) logger

   Log activity on a DALI bus to UART/RTT.

Overview
********

This sample utilizes the :ref:`dali <dali_api>` driver API to log DALI frames to UART/RTT.

The output format conforms to `dali_mon specification <https://github.com/SvenHaedrich/dali_mon/blob/main/docs/serial.md>`_.

Building and Running
********************

The interface to the DALI bus is defined in the board's devicetree. For details see the
:zephyr:code-sample:`DALI blinky <dali_blinky>` sample.

.. note:: For proper operation a DALI specific physical interface is required.

Building and Running for Nordic nRF52840
========================================

The :zephyr_file:`samples/drivers/dali/logger/boards/nrf52840dk_nrf52840.overlay` is specifically
for the Mikroe-2672 DALI2 click development board used as physical interface to the DALI bus. This
board uses negative logic for signal transmission (Tx Low <-> DALI Bus Idle).

The sample can be built and executed for the :zephyr:board:`nrf52840dk` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dali/logger
   :board: nrf52840dk/nrf52840
   :goals: build flash
   :compact:

Sample output
=============

You should see log on UART/RTT, when there's activity on the bus.
