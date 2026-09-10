.. SPDX-FileCopyrightText: Copyright (c) 2026 Texas Instruments Incorporated
.. SPDX-License-Identifier: Apache-2.0

.. zephyr:board:: lp_mspm33c321a

Overview
********

The MSPM33C321A is part of the TI MSP highly integrated, ultra-low-power 32-bit MCU
family based on the Arm® Cortex®-M33 32-bit core platform operating at up to 160 MHz.
These MCUs offer high-performance analog peripheral integration, support extended
temperature ranges from -40 °C to 125 °C, and operate with supply voltages from
1.62 V to 3.6 V.

The MSPM33C321A provides 1 MB embedded flash program memory with built-in error
correction code (ECC) and 256 KB SRAM with hardware parity option. These MCUs also
incorporate a memory protection unit, DMA, and a variety of peripherals including:

* Analog.

  * One 12-bit high-speed SAR ADC (HSADC) with 4 sequencers and up to 32 input
    channels, oversampling (up to 8x), and internal voltage reference (1.4 V, 2.5 V).

* Digital.

  * Multiple timer groups (TIMG) with alarm and capture/compare support.

  * Up to 3 GPIO ports (GPIOA, GPIOB, GPIOC) with interrupt support.

  * DMA controller with up to 16 channels.

* Communication.

  * UNICOMM (Universal Communication) peripherals supporting UART, SPI, and I2C
    modes, configurable per instance.

See the `MSPM33C321A Datasheet`_ and `MSPM33C321A TRM`_ for full device specifications.

Hardware
********

The LP-MSPM33C321A LaunchPad is a low-cost development board for the MSPM33C321A MCU.

Board features:

- MSPM33C321A MCU (Arm Cortex-M33 @ 160 MHz, 1 MB flash, 256 KB SRAM)
- On-board XDS110 debug probe with SWD interface
- Three user LEDs (red, green, blue) and two push buttons
- Multiple expansion headers for BoosterPack ecosystem compatibility
- On-board 32.768 kHz crystal for LFCLK

Details on the LP-MSPM33C321A LaunchPad can be found on the
`TI LP-MSPM33C321A Product Page`_.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

LEDs
----

* **LED0** (red)   = **PC26**
* **LED1** (green) = **PC27**
* **LED2** (blue)  = **PA2**

Push Buttons
------------

* **S1** = reset
* **S2** (user) = **PB21**

Serial Port
-----------

The Zephyr console is assigned to UNICOMM12 UART (TX=PA10, RX=PA11).
Default settings: 115200 8N1, no flow control. Accessible via the XDS110
debugger's virtual COM port over USB.

System Clock
------------

The MSPM33C321A clock tree is driven by SYSOSC (32 MHz) through the system PLL:

- SYSOSC (32 MHz) → SYSPLL → HSCLK → MCLK (160 MHz, CPU clock)
- ULPCLK: 40 MHz ultra-low-power clock for peripherals
- LFCLK: 32.768 kHz low-frequency clock (from on-board crystal)

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

.. note::

   OpenOCD support for MSPM33 is **not yet upstream** in the OpenOCD project.
   A custom TI OpenOCD fork must be built before using ``west flash`` or
   ``west debug`` with this board. See `Building a patched OpenOCD`_ below.

Building
========

Follow the :ref:`getting_started` instructions for Zephyr application development.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: lp_mspm33c321a
   :goals: build

Flashing
========

Once the custom OpenOCD is built, flash using:

.. code-block:: console

   $ west flash --openocd <path-to-openocd>/src/openocd \
         --openocd-search <path-to-openocd>/tcl

After flashing, a reset is required to start the application. Either press the
nRST button on the board, or run:

.. code-block:: console

   $ <path-to-openocd>/src/openocd -s <path-to-openocd>/tcl \
         -f board/ti_mspm33_launchpad.cfg \
         -c "init; mspm33_board_reset; shutdown"

When multiple boards are connected, identify each board's XDS110 serial number
and pass it to the adapter:

.. code-block:: console

   $ lsusb -v -d 0451:bef3 | grep iSerial
   $ west flash --openocd <path-to-openocd>/src/openocd \
         --openocd-search <path-to-openocd>/tcl \
         --cmd-pre-init "adapter serial <serial_number>"

Debugging
=========

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: lp_mspm33c321a
   :goals: debug

Pass the same ``--openocd`` and ``--openocd-search`` arguments as for flashing.

Building a patched OpenOCD
==========================

MSPM33 OpenOCD support is provided via a TI fork of OpenOCD. Build it as follows:

1. Install prerequisites (Ubuntu):

   .. code-block:: console

      $ sudo apt install autoconf automake libtool pkg-config \
            libusb-1.0-0-dev libhidapi-dev

2. Clone and build:

   .. code-block:: console

      $ git clone -b ti-release https://github.com/TexasInstruments/ti-openocd
      $ cd ti-openocd
      $ git submodule update --init --recursive
      $ ./bootstrap
      $ ./configure --enable-xds110
      $ make

Pre-built binaries for Linux and Windows are available from the
`TI OpenOCD ti-v1.2.0 release page`_.

Recovery
========

The TI OpenOCD build provides device recovery commands:

**Factory reset** (recover a locked or bricked board):

.. code-block:: console

   $ <path-to-openocd>/src/openocd -s <path-to-openocd>/tcl \
         -f board/ti_mspm33_launchpad.cfg \
         -c "init; mspm33_factory_reset; shutdown"

**Mass erase** (erase all flash):

.. code-block:: console

   $ <path-to-openocd>/src/openocd -s <path-to-openocd>/tcl \
         -f board/ti_mspm33_launchpad.cfg \
         -c "init; mspm33_mass_erase; shutdown"

References
**********

.. target-notes::

.. _TI LP-MSPM33C321A Product Page:
   https://www.ti.com/tool/LP-MSPM33C321A

.. _MSPM33C321A Datasheet:
   https://www.ti.com/product/MSPM33C321A

.. _MSPM33C321A TRM:
   https://www.ti.com/lit/ug/slau962/slau962.pdf

.. _TI OpenOCD ti-v1.2.0 release page:
   https://github.com/TexasInstruments/ti-openocd/releases/tag/ti-v1.2.0
