.. zephyr:board:: rh02

Overview
********

The Jhoinrch RH02 is a dedicated USB to CAN adapter board. More information can be
found on the `Jhoinrch website`_.

Hardware
********

The Jhoinrch RH02 is equipped with a STM32F072CB microcontroller and features a USB-A plug, a
terminal block for connecting to the CAN bus, three LEDs (TX, RX, PWR), a ``BOOT`` switch, and a switch
for enabling a 120 Ω CAN termination resistor. It advertises a max speed of 1 Mbps.

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The STM32F072CB PLL is driven by an external crystal oscillator (HSE) running at 24 MHz and
configured to provide a system clock of 48 MHz.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and :ref:`application_run`
for more details). For activating USB DFU via the ROM bootloader, flick the ``BOOT`` switch
down while plugging into a USB port prior to flashing.

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: rh02
   :goals: flash

.. _Jhoinrch website:
   https://jhoinrch.net/index.php?m=home&c=View&a=index&aid=124
