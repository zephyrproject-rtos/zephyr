.. _ch32x035evt:

WCH CH32X035EVT
###############

Overview
********

The WCH CH32X035EVT is the official evaluation board for the CH32X035
family (QingKe V4C RISC-V core running at a fixed 48 MHz HSI clock).
The board exposes the CH32X035 peripherals and provides a user LED on
PA0 and a USART1 console on PB10 (TX) / PB11 (RX) at 115200 baud.

Programming
***********

WCH-LinkE (wlink runner)
========================

Using a WCH-LinkE debug probe connected to the SWD pins:

.. code-block:: console

   west build -b ch32x035evt samples/hello_world
   west flash --runner wlink

Debugging
=========

This board can be debugged via OpenOCD or ``minichlink``. Console messages
are written to USART1 by default, but can also be sent via the debug module
and read by an SDI compatible debugger such as ``minichlink``. See
:kconfig:option:`CONFIG_WCH_SDI_CONSOLE` for more.

.. note::

   A board picture is still to be added before submitting this board
   upstream.
