..
   SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
   SPDX-License-Identifier: Apache-2.0

.. _snippet-rp2040-200mhz:

RP2040 200 MHz clock (rp2040-200mhz)
####################################

Configure RP2040 system and peripheral clocks to 200 MHz using a 12 MHz crystal oscillator.

.. code-block:: console

   west build -b rpi_pico -S rp2040-200mhz samples/hello_world
