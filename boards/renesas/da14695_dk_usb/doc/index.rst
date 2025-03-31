.. zephyr:board:: da14695_dk_usb

DA14695 Development Kit USB
###########################

Overview
********

The DA14695 Development Kit USB is a low cost development board for DA14695.
The development kit comes with an integrated debugger and an USB hub
to have both the on-chip USB and the J-Link connected via a single port.

Hardware
********

DA14695 Development Kit USB has two external oscillators. The frequency of
the sleep clock is 32768 Hz. The frequency of the system clock is 32 MHz.

Supported Features
==================

.. zephyr:board-supported-hw::

For more information about the DA14695 Development Kit see:

- `DA14695 DK USB website`_

System Clock
============

The DA14695 Development Kit USB is configured to use the 32 MHz external oscillator
on the board.

Connections and IOs
===================

The DA14695 Development Kit USB has one LED and one push button which can be used
by applications. The UART is connected to on-board serial converter and accessible
via USB1 port on motherboard.

The pin connections are as follows:

* LED (red), = P1.01
* BUTTON, labeled k1 = P0.06
* UART RX, connected to J-Link serial = P0.08
* UART TX, connected to J-Link serial = P0.09

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``da14695_dk_usb`` board configuration can be
built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

Flashing
========

The DA14695 boots from an external flash connected to QSPI interface. The image
written to flash has to have proper header prepended. The process is simplified
by using dedicated `eZFlashCLI`_ tool that takes care of writing header and can
handle different types of flash chips connected to DA1469x MCU. Follow instructions
on `ezFlashCLI`_ to install the tool. Once installed, flashing can be done in the
usual way.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: da14695_dk_usb
   :goals: build flash

Debugging
=========

The DA14695 Development Kit USB includes a `J-Link`_ adaptor built-in
which provides both debugging interface and serial port.
Application can be debugged in the usual way once DA14695 Development Kit USB
is connected to PC via USB.

References
**********

.. target-notes::

.. _DA14695 DK USB website: https://www.renesas.com/us/en/products/wireless-connectivity/bluetooth-low-energy/da14695-00hqdevkt-u-smartbond-da14695-bluetooth-low-energy-52-usb-development-kit
.. _DA1469x Datasheet: https://www.renesas.com/eu/en/document/dst/da1469x-datasheet
.. _J-Link: https://www.segger.com/jlink-debug-probes.html
.. _ezFlashCLI: https://github.com/ezflash/ezFlashCLI/
