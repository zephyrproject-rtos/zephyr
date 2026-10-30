.. zephyr:code-sample:: dali_logger
   :name: Digital Addressable Lighting Interface (DALI) logger

   Log activity on a DALI bus to UART/RTT.

Overview
********

This sample utilizes the :ref:`dali <dali_api>` driver API to log DALI frames to UART/RTT.

Each frame is logged in the serial format of the `DALI USB interface`_, which the `dali_mon`_
tool decodes.

Building and Running
********************

The interface to the DALI bus is defined in the board's devicetree. For details see the
:zephyr:code-sample:`DALI blinky <dali>` sample.

.. note:: For proper operation a DALI specific physical interface is required.

Building and Running for ST Nucleo F091RC
=========================================
The :ref:`mikroe_dali_2_click_shield` is used as physical interface to the DALI bus. The board
specific settings are provided by the shield in
:zephyr_file:`boards/shields/mikroe_dali_2_click/boards/nucleo_f091rc.overlay`. The click board
uses negative logic for signal transmission (Tx Low <-> DALI Bus Idle). The DALI 2 Click board is
placed in the Arduino UNO click shield's slot 1.

The sample can be built and executed for the :zephyr:board:`nucleo_f091rc` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dali/logger
   :board: nucleo_f091rc
   :goals: build flash
   :shield: arduino_uno_click,mikroe_dali_2_click
   :compact:

Building and Running for Nordic nRF52840
========================================
The :ref:`mikroe_dali_2_click_shield` is used as physical interface to the DALI bus. The board
specific settings are provided by the shield in
:zephyr_file:`boards/shields/mikroe_dali_2_click/boards/nrf52840dk_nrf52840.overlay`. The pin
assignment supports the use of an Arduino UNO click shield. The click board uses negative logic
for signal transmission (Tx Low <-> DALI Bus Idle). The DALI 2 Click board is placed in the Arduino
UNO click shield's slot 1.

The sample can be built and executed for the :zephyr:board:`nrf52840dk` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dali/logger
   :board: nrf52840dk/nrf52840
   :goals: build flash
   :shield: arduino_uno_click,mikroe_dali_2_click
   :compact:

Sample output
=============

The sample logs a line at startup, then one line per frame or event on the bus:

.. code-block:: console

   [00:00:00.001,000] <inf> dali_logger: DALI logger started
   [00:00:12.782,000] <inf> dali_logger: {000031ee:10 0000ff05}
   [00:00:14.845,000] <inf> dali_logger: {000039fd:10 0000ff06}

The output can be piped into ``dali_mon``, which ignores lines without a frame:

.. code-block:: console

   $ dali_mon < /dev/ttyACM0
   12.782 |    0.000 |     FF05 | BC GEAR              RECALL MAX LEVEL
   14.845 |    2.063 |     FF06 | BC GEAR              RECALL MIN LEVEL

References
**********

.. target-notes::

.. _DALI USB interface:
   https://github.com/SvenHaedrich/dali_usb_lpc1114/blob/main/doc/messages.md

.. _dali_mon:
   https://github.com/SvenHaedrich/dali_mon
