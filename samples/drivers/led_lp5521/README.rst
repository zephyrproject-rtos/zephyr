.. zephyr:code-sample:: lp5521
   :name: LP5521 RGB LED
   :relevant-api: led_interface

   Control 3 RGB LEDs connected to an LP5521 driver chip.

Overview
********

This sample controls 3 LEDs connected to a TI LP5521 driver. The sample turns all
LEDs on and switches all LEDs off again within a one second interval.

Refer to the `LP5521 Manual`_ for the RGB LED connections and color channel mappings used
by this sample.

Building and Running
********************

Build the application for the :ref:`croxel_cx1825_nrf52840` board, which includes an
LP5521 LED driver.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led_LP5521
   :board: croxel_cx1825_nrf52840
   :goals: build
   :compact:

For flashing the application, refer to the Flashing section of the :ref:`croxel_cx1825_nrf52840`
board documentation.

.. _LP5521 Manual: https://www.ti.com/lit/ds/symlink/lp5521.pdf
