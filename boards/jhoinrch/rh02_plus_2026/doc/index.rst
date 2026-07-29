.. zephyr:board:: rh02_plus_2026

Overview
********

The Jhoinrch RH02 Plus (2026) is a dedicated USB to CAN FD adapter board. More information can be
found on the `Jhoinrch website`_.

Hardware
********

The Jhoinrch RH02 Plus (2026) is equipped with a STM32G431CB microcontroller and features a USB-A plug, a
terminal block for connecting to the CAN bus, three LEDs (PWR, STATE, WORK), a ``BOOT`` switch, and a switch
for enabling a 120 Ω CAN termination resistor. It advertises a max speed of 5 Mbps.

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The STM32G431CB PLL is driven by an external crystal oscillator (HSE) running at 25 MHz and
configured to provide a system clock of 160 MHz. This allows generating a FDCAN1 core clock of 80
MHz.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and :ref:`application_run`
for more details). For activating USB DFU via the ROM bootloader, flick the ``BOOT`` switch
down while plugging into a USB port prior to flashing.

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: rh02_plus_2026
   :goals: flash

.. _Jhoinrch website:
   https://jhoinrch.net/index.php?m=home&c=View&a=index&aid=123
