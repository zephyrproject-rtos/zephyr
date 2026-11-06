.. _lcm_rgb_5inch:

NXP LCM RGB 5" 800x480 Parallel RGB Panel
#########################################

Overview
********

The LCM-RGB-5INCH is a 5” 800x480 IPS TFT LCD panel which connects to the
board's dedicated LCM 40 pin connector, with Parallel RGB display interface
and touch sensing controller.

Requirements
************

This shield can only be used with a board that wires its LCD controller DPI signals
and the two I2C GPIO expanders to the LCM connector, such as
:ref:`mimxrt2660_evk`.

Programming
***********

Set ``--shield lcm_rgb_5inch`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/display
   :board: mimxrt2660_evk/mimxrt2663/cm85
   :shield: lcm_rgb_5inch
   :goals: build

.. include:: ../../../nxp/common/board-footer.rst.inc
