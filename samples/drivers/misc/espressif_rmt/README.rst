.. zephyr:code-sample:: rmt
   :name: Espressif Remote Control Transceiver (RMT)
   :relevant-api: rmt_interface

   Generate a WS2812 signal using the RMT driver API.

Overview
********

This sample demonstrates how to use the :ref:`Espressif RMT driver API <rmt_api>`.

You  should connect a WS2812 strip to the RMT output of the board.

Building and Running
********************

The RMT output is defined in the board's devicetree and pinmux file.

Building and Running for esp32-devkitc
======================================
The sample can be built and executed for the
:zephyr:board:`esp32_devkitc` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/misc/espressif_rmt
   :board: esp32_devkitc/esp32/procpu
   :goals: build flash
   :compact:

Sample output
=============

.. code-block:: console

   *** Booting Zephyr OS build v4.3.0-2805-g63ff14e3d8d0 ***
   Create RMT TX channel
   I (141) gpio: GPIO[12]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0 
   Install led strip encoder
   Enable RMT TX channel
   Start LED rainbow chase

After you seeing this log, you should see a rainbow chasing demonstration pattern.
