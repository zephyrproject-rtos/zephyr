.. zephyr:board:: ai_wb2_12f_kit
.. |board| replace:: ``ai_wb2_12f_kit``
.. |soc| replace:: BL602

Overview
********

This WB2 (BL602) 12F format Module Development Board features a BL602/BL604 SoC.

.. include:: ../../../aithinker/common/soc-bl60x.rst.inc

Serial Port
===========

The |board| board uses UART0 as default serial port.  It is connected
to USB Serial converter and port is used for both program and console.

.. include:: ../../../aithinker/common/building-flashing.rst.inc

.. code-block:: console

   *** Booting Zephyr OS build v4.1.0 ***
   Hello World! ai_wb2_12f_kit/bl602c00q2i

Congratulations, you have |board| configured and running Zephyr.

.. _Schematics:
   https://docs.ai-thinker.com/en/wb2
