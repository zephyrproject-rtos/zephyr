.. zephyr:board:: da1469x_dk_pro

DA1469x Development Kit Pro
###########################

Overview
********

The DA1469x Development Kit Pro hardware provides support for the Renesas
DA1469x ARM Cortex-M33 MCU family. The development kit consist of a motherboard
with connectors and integrated debugger and an interchangeable daughterboard
with an actual MCU (e.g. DA14695 or DA14699).

Hardware
********

DA1469x Development Kit Pro has two external oscillators. The frequency of
the sleep clock is 32768 Hz. The frequency of the system clock is 32 MHz.

Supported Features
==================

.. zephyr:board-supported-hw::

For more information about the DA14695 Development Kit see:

- `DA14695 DK website`_
- `DA14699 daughterboard website`_

System Clock
============

The DA1469x Development Kit Pro is configured to use the 32 MHz external oscillator
on the board.

Connections and IOs
===================

The DA1469x Development Kit Pro has one LED and one push button which can be used
by applications. The UART is connected to on-board serial converter and accessible
via USB1 port on motherboard.

The pin connections are as follows:

* LED (red), located on daughterboard = P1.01
* BUTTON, located on motherboard = P0.06
* UART RX, via USB1 on motherboard = P0.08
* UART TX, via USB1 on motherboard = P0.09

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``da1469x_dk_pro`` board configuration can be
built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

Flashing
========

The DA1469x boots from an external flash connected to QSPI interface. The image
written to flash has to have proper header prepended. The process is simplified
by using dedicated `eZFlashCLI`_ tool that takes care of writing header and can
handle different types of flash chips connected to DA1469x MCU. Follow instructions
on `ezFlashCLI`_ to install the tool. Once installed, flashing can be done in the
usual way.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: da1469x_dk_pro
   :goals: build flash

Debugging
=========

The DA1469x Development Kit Pro includes a `J-Link`_ adaptor built-in on
motherboard which provides both debugging interface and serial port.
Application can be debugged in the usual way once DA1469x Development Kit Pro
is connected to PC via USB port on motherboard.

References
**********

.. target-notes::

.. _DA14695 DK website: https://www.renesas.com/eu/en/products/interface-connectivity/wireless-communications/bluetooth-low-energy/da14695-00hqdevkt-p-smartbond-da14695-bluetooth-low-energy-52-development-kit-pro
.. _DA14699 daughterboard website: https://www.renesas.com/br/en/products/interface-connectivity/wireless-communications/bluetooth-low-energy/da14699-00hrdb-p-smartbond-da14695-bluetooth-low-energy-52-development-kit-pro-vfbga100-daughterboard
.. _DA1469x Datasheet: https://www.renesas.com/eu/en/document/dst/da1469x-datasheet
.. _J-Link: https://www.segger.com/jlink-debug-probes.html
.. _ezFlashCLI: https://github.com/ezflash/ezFlashCLI/
