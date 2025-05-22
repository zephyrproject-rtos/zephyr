.. _tianma-tm070jvgh3:

.. Copyright (c) 2019-2023 TQ-Systems GmbH <license@tq-group.com>
.. SPDX-License-Identifier: CC-BY-4.0
.. Author: Isaac L. L. Yuki

TQ-SYSTEMS USES TIANMA-TM070JVGH33-DISPLAY
##########################################

Overview
********

The Tianma 7" (17.78cm) wide TFT display TM070JVHG33-01 with WXGA
resolution provides top features: The integrated capacitive touch
screen allows for easy operation with up to 5 fingers and is protected
by a 1.1mm chemically hardened front glass with black passepartout.

The SFT technology provides a viewing angle of v/h 170°/170° and
enables a good readability of the content from all directions.
As usual, Tianma guarantees a long-term availability for thius
display.

Requirements
************

This shield can only be used with the TQ-SYSTEMS MBa117XL board.
The display is connected through the SN65DSI83 MIPI-DSI/LVDS bridge.

***********

Set ``--shield mba117xl_tm070`` when you invoke ``west build``. For
example:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/display
   :board: mba117xl/mimxrt1176/cm7
   :shield: mba117xl_tm070
   :goals: build

References
**********

.. target-notes::

.. _tianma-tm070jvgh33 product page:
   https://tianma.eu

.. _sn65dsi83 product page:
   https://www.ti.com/product/SN65DSI83
