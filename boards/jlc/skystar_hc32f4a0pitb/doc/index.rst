.. zephyr:board:: skystar_hc32f4a0pitb

Overview
********

The SkyStar-HC32F4A0PITB board is a JLC development board based on the
XHSC HC32F4A0PITB Cortex-M4 MCU.

For more information about the SkyStar-HC32F4A0PITB board:

- `SkyStar HC32F4A0PITB Board Wiki`_

Supported Features
==================

.. zephyr:board-supported-hw::

Serial Console
==============

The default console uses USART1 at 115200 baud, 8 data bits, no parity, and
1 stop bit. Connect the USB-to-TTL adapter RX pin to PA9, TX pin to PA10,
and GND to GND.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flash with pyOCD:

.. code-block:: console

   west flash --runner pyocd

Install a pyOCD target pack that provides ``hc32f4a0pitb`` before flashing.

.. _SkyStar HC32F4A0PITB Board Wiki:
   https://wiki.lckfb.com/zh-hans/tkx/tkx-hc32f4a0pitb/
