.. zephyr:board:: glyph_c6

Overview
********

Glyph-C6 is powered by ESP32-C6 SoC.
It consists of a high-performance (HP) 32-bit RISC-V processor, which can be clocked up to 160 MHz,
and a low-power (LP) 32-bit RISC-V processor, which can be clocked up to 20 MHz.
It has a 512KB SRAM, and works with 4MB external SPI flash.
For more information, check `Glyph-C6`_.

Hardware
********

Most of the I/O pins are broken out to the pin headers on both sides for easy interfacing.
Developers can either connect peripherals with jumper wires or mount glyph c6 on
a breadboard.

.. include:: ../../../espressif/common/soc-esp32c6-features.rst
   :start-after: espressif-soc-esp32c6-features

Supported Features
==================

.. zephyr:board-supported-hw::

System Requirements
*******************

.. include:: ../../../espressif/common/system-requirements.rst
   :start-after: espressif-system-requirements

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

.. include:: ../../../espressif/common/building-flashing.rst
   :start-after: espressif-building-flashing

.. include:: ../../../espressif/common/board-variants.rst
   :start-after: espressif-board-variants

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

References
**********

.. target-notes::

.. _`Glyph-C6`: https://learn.pcbcupid.com/boards/glyph-c6/overview
