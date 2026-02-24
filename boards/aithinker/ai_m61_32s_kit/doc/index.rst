.. zephyr:board:: ai_m61_32s_kit
.. |board| replace:: ``ai_m61_32s_kit``
.. |soc| replace:: BL618
.. |safe_overclock| replace:: ``ai_m61_32s_kit/bl618m05q2i/safe_overclock``
.. |unsafe_overclock| replace:: ``ai_m61_32s_kit/bl618m05q2i/unsafe_overclock``

Overview
********

Ai-M61-32S is a Wi-Fi 6 + BLE5.3 module developed by Shenzhen Ai-Thinker Technology
Co., Ltd.

.. include:: ../../../aithinker/common/soc-bl61x.rst.inc

If you are using the ALL variant, please use: ``ai_m61_32s_kit@ALL``.

Serial Port
===========

The |board| board uses UART0 as default serial port.  It is connected
to USB Serial converter and port is used for both program and console.

.. include:: ../../../aithinker/common/building-flashing.rst.inc

.. code-block:: console

   *** Booting Zephyr OS build v4.3.0 ***
   Hello World! ai_m61_32s_kit/bl618m05q2i

Congratulations, you have |board| configured and running Zephyr.

.. _Schematics:
   https://docs.ai-thinker.com/en/ai_m61/
