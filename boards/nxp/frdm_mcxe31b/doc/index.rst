.. zephyr:board:: frdm_mcxe31b

Overview
********
The FRDM-MCXE31B board is a design and evaluation platform based on the NXP MCXE31B
microcontroller (MCU). NXP MCXE31B MCU based on an Arm Cortex-M7 core, running at
speeds of up to 160 MHz with a 2.97 to 5.5V supply.

Hardware
********

- MCXE31B Arm Cortex-M7 microcontroller running up to 160 MHz
- 4MB dual-bank on chip Flash
- 320KB SRAM + 192KB TCM
- 2x I2C
- 6x SPI
- 16x UART
- On-board MCU-Link debugger with CMSIS-DAP
- Arduino Header, mikroBUS

For more information about the MCXE31B SoC and FRDM-MCXE31B board, see:

- `MCXE31X Datasheet`_
- `MCXE31X Reference Manual`_
- `FRDM-MCXE31B Board User Manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Each GPIO port is divided into two banks: low bank, from pin 0 to 15, and high
bank, from pin 16 to 31. For example, ``PTA2`` is the pin 2 of ``gpioa_l`` (low
bank), and ``PTA20`` is the pin 4 of ``gpioa_h`` (high bank).

The GPIO controller provides the option to route external input pad interrupts
to either the SIUL2 EIRQ or WKPU interrupt controllers, as supported by the SoC.
By default, GPIO interrupts are routed to SIUL2 EIRQ interrupt controller,
unless they are explicitly configured to be directed to the WKPU interrupt
controller, as outlined in :zephyr_file:`dts/bindings/gpio/nxp,siul2-gpio.yaml`.

To find information about which GPIOs are compatible with each interrupt
controller, refer to the device reference manual.

+-------+-------------+---------------------------+
| Name  | Function    | Usage                     |
+=======+=============+===========================+
| PTC16 | GPIO        | Red LED                   |
+-------+-------------+---------------------------+
| PTB22 | GPIO        | Green LED                 |
+-------+-------------+---------------------------+
| PTC14 | GPIO        | Blue LED                  |
+-------+-------------+---------------------------+
| PTE3  | LPUART5_RX  | UART Console              |
+-------+-------------+---------------------------+
| PTE14 | LPUART5_TX  | UART Console              |
+-------+-------------+---------------------------+

System Clock
============

The MCXE31B SoC is configured to use PLL running at 160MHz as a source for
the system clock.

Serial Port
===========

The MCXE31B LPUART5 is used for the console.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Configuring a Debug Probe
=========================

A debug probe is used for both flashing and debugging the board. This board is
configured by default to use the MCU-Link CMSIS-DAP Onboard Debug Probe.

Using LinkServer
----------------

Linkserver is the default runner for this board, and supports the factory
default MCU-Link firmware. Follow the instructions in
:ref:`mcu-link-cmsis-onboard-debug-probe` to reprogram the default MCU-Link
firmware. This only needs to be done if the default onboard debug circuit
firmware was changed. To put the board in ``ISP mode`` to program the firmware,
short jumper JP3.

Using J-Link
------------

There are two options. The onboard debug circuit can be updated with Segger
J-Link firmware by following the instructions in
:ref:`mcu-link-jlink-onboard-debug-probe`.
To be able to program the firmware, you need to put the board in ``ISP mode``
by shorting the jumper JP3.
The second option is to attach a :ref:`jlink-external-debug-probe` to the
10-pin SWD connector (J14) of the board.
For both options use the ``-r jlink`` option with west to use the jlink runner.

.. code-block:: console

   west flash -r jlink

Configuring a Console
=====================

Connect a USB cable from your PC to J13, and use the serial terminal of your choice
(minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flashing
========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_mcxe31b
   :goals: flash

Open a serial terminal, reset the board (press the RESET button), and you should
see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0-2092-g17e93a718422 ***
   Hello World! frdm_mcxe31b/mcxe31b

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_mcxe31b
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0-2092-g17e93a718422 ***
   Hello World! frdm_mcxe31b/mcxe31b

Troubleshooting
===============

.. include:: ../../common/segger-ecc-systemview.rst.inc

.. include:: ../../common/board-footer.rst.inc

.. _MCXE31X Datasheet:
   https://www.nxp.com/docs/en/data-sheet/MCXEP172M160FB0.pdf

.. _MCXE31X Reference Manual:
   https://www.nxp.com/webapp/Download?colCode=MCXE31XRM&location=null

.. _FRDM-MCXE31B Board User Manual:
   https://www.nxp.com/webapp/Download?colCode=UM12330&location=null&isHTMLorPDF=HTML
