.. zephyr:board:: usb2canfdv2

Overview
********

The WeAct Studio USB2CANFDV2 is a dedicated USB to CAN FD adapter board. More information can be
found on the `USB2CANFDV2 website`_.

Hardware
********

The USB2CANFDV2 is equipped with a STM32G431CB microcontroller and features a USB-C connector, a
terminal block for connecting to the CAN bus, three LEDs, a ``BOOT0`` push-button, and a slide
switch for enabling a 120 Ω CAN termination resistor.

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The STM32G431CB PLL is driven by an external crystal oscillator (HSE) running at 16 MHz and
configured to provide a system clock of 160 MHz. This allows generating a FDCAN1 core clock of 80
MHz.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and :ref:`application_run`
for more details). For activating USB DFU via the ROM bootloader, keep the ``BOOT0`` push-button
pressed while plugging in the USB cable prior to flashing.

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: usb2canfdv2
   :goals: flash

.. _USB2CANFDV2 website:
   https://github.com/WeActStudio/WeActStudio.USB2CANFDV2
