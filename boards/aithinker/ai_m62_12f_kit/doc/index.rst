.. zephyr:board:: ai_m62_12f_kit
.. |board| replace:: ``ai_m61_12f_kit``
.. |soc| replace:: BL616
.. |safe_overclock| replace:: ``ai_m61_12f_kit/bl616c50q2i/safe_overclock``
.. |unsafe_overclock| replace:: ``ai_m61_12f_kit/bl616c50q2i/unsafe_overclock``

Overview
********

Ai-M62-12F is a Wi-Fi 6 + BLE5.3 module developed by Shenzhen Ai-Thinker Technology
Co., Ltd.

.. include:: ../../../aithinker/common/soc-bl61x.rst.inc

If you are using the ALL variant, please use: ``ai_m62_12f_kit@ALL``.

Serial Port
===========

The |board| board uses UART0 as default serial port.  It is connected
to USB Serial converter and port is used for both program and console.

.. include:: ../../../aithinker/common/building-flashing.rst.inc

.. code-block:: console

   *** Booting Zephyr OS build v4.3.0 ***
   Hello World! ai_m62_12f_kit/bl616c50q2i

Congratulations, you have |board| configured and running Zephyr.

.. _Schematics:
   https://docs.ai-thinker.com/en/ai_m62/
