.. zephyr:board:: ch582m_dev_board

Overview
********

The `WCH`_ CH582M Dev Board is a generic evaluation target for the CH582M, a
QingKe V4A 32-bit RISC-V SoC with an integrated BLE 5.3 radio. This board
target enables the following peripherals:

* CLOCK
* :abbr:`GPIO (General Purpose Input Output)`
* :abbr:`NVIC (Nested Vectored Interrupt Controller)`
* :abbr:`UART (Universal Asynchronous Receiver Transmitter)`
* :abbr:`LED (Light Emitting Diode)`

Hardware
********

The QingKe V4A 32-bit RISC-V core of the CH582M runs at 6.4 MHz out of reset
(32 MHz crystal divided by 5). The CH582M provides 512 KB of flash and 32 KB
of SRAM.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
====================

UART0 is used as the default console. It is available on PB4 (RXD) and PB7
(TXD), which is the peripheral's default pin location after reset, so no pin
remapping is required.

Programming and Debugging
**************************

.. zephyr:board-supported-runners::

Applications for the ``ch582m_dev_board`` board target can be built in the usual way
(see :ref:`build_an_application` and :ref:`application_run` for more
details).

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ch582m_dev_board
   :goals: build

Flashing and debugging require an external SWD/SDI programmer, such as the
WCH-Link, and OpenOCD with WCH-specific support.

It's also possible to flash image using WCHISPStudio using default first stage bootloader through
USB-C port. To do this add image to WCHISPStudio, press BOOT button on the board and plug in USB-C.

References
**********

.. target-notes::

.. _WCH: http://www.wch-ic.com
