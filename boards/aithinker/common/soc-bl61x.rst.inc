.. SPDX-FileCopyrightText: Copyright MASSDRIVER EI (massdriver.space)
.. SPDX-License-Identifier: Apache-2.0

It is equipped with |soc| chip as the core processor, supports Wi-Fi
802.11b/g/n/ax protocol and BLE protocol, and supports Thread protocol. The |soc| system
includes a low-power 32-bit RISC-V CPU with floating-point unit, DSP unit, cache and
memory, with a maximum dominant frequency of 320M.

Hardware
********

For more information about the Bouffalo Lab |soc| MCU:

- `Bouffalo Lab BL61x MCU Datasheet`_
- `Bouffalo Lab Development Zone`_
- |board| `Schematics`_

Supported Features
==================

.. zephyr:board-supported-hw::


System Clock
============

This |soc| development board is configured to run at maximum speed (320MHz)
and can be overclocked to 480 MHz.

This board provides demonstration configurations for the overclocking of the |soc| SoC clocks:

- |safe_overclock| demonstrates 120MHz BCLK and 480MHz core clock on both variants.
  This nets a Coremark score up to 1600.

- |unsafe_overclock| demonstrates 120MHz or 106MHz BCLK and 640MHz core clock with higher core voltages.
  This nets a Coremark score up to 2100.

When overclocking, DMA and DMA-using drivers will not work properly.

.. _Bouffalo Lab BL61x MCU Datasheet:
	https://github.com/bouffalolab/bl_docs/tree/main/BL616_DS/en

.. _Bouffalo Lab Development Zone:
	https://dev.bouffalolab.com/home?id=guest
