.. _mimxrt1020_evk:

NXP MIMXRT1020-EVK
##################

Overview
********

The i.MX RT1020 expands the i.MX RT crossover processor families by providing
high-performance feature set in low-cost LQFP packages, further simplifying
board design and layout for customers. The i.MX RT1020 runs on the Arm®
Cortex®-M7 core at 500 MHz.

.. image:: mimxrt1020_evk.jpg
   :width: 720px
   :align: center
   :alt: MIMXRT1020-EVK

Hardware
********

- MIMXRT1021DAG5A MCU

- Memory

  - 256 Mbit SDRAM
  - 64 Mbit QSPI Flash
  - TF socket for SD card

- Connectivity

  - 10/100 Mbit/s Ethernet PHY
  - Micro USB host and OTG connectors
  - CAN transceivers
  - Arduino interface

- Audio

  - Audio Codec
  - 4-pole audio headphone jack
  - Microphone
  - External speaker connection

- Power

  - 5 V DC jack

- Debug

  - JTAG 20-pin connector
  - OpenSDA with DAPLink

For more information about the MIMXRT1020 SoC and MIMXRT1020-EVK board, see
these references:

- `i.MX RT1020 Website`_
- `i.MX RT1020 Datasheet`_
- `i.MX RT1020 Reference Manual`_
- `MIMXRT1020-EVK Website`_
- `MIMXRT1020-EVK User Guide`_
- `MIMXRT1020-EVK Design Files`_

Supported Features
==================

The mimxrt1020_evk board configuration supports the following hardware
features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+

The default configuration can be found in the defconfig file:
``boards/arm/mimxrt1020_evk/mimxrt1020_evk_defconfig``

Other hardware features are not currently supported by the port.

Connections and I/Os
====================

The MIMXRT1020 SoC has five pairs of pinmux/gpio controllers.

+---------------+-----------------+---------------------------+
| Name          | Function        | Usage                     |
+===============+=================+===========================+
| GPIO_AD_B0_05 | GPIO            | LED                       |
+---------------+-----------------+---------------------------+
| GPIO_AD_B0_06 | LPUART1_TX      | UART Console              |
+---------------+-----------------+---------------------------+
| GPIO_AD_B0_07 | LPUART1_RX      | UART Console              |
+---------------+-----------------+---------------------------+
| GPIO_AD_B1_08 | LPUART2_TX      | UART BT HCI               |
+---------------+-----------------+---------------------------+
| GPIO_AD_B1_09 | LPUART2_RX      | UART BT HCI               |
+---------------+-----------------+---------------------------+
| WAKEUP        | GPIO            | SW0                       |
+---------------+-----------------+---------------------------+

System Clock
============

The MIMXRT1020 SoC is configured to use the 24 MHz external oscillator on the
board with the on-chip PLL to generate a 500 MHz core clock.

Serial Port
===========

The MIMXRT1020 SoC has eight UARTs. ``LPUART1`` is configured for the console,
``LPUART2`` for the Bluetooth Host Controller Interface (BT HCI), and the
remaining are not used.

Programming and Debugging
*************************

The MIMXRT1020-EVK includes the :ref:`nxp_opensda` serial and debug adapter
built into the board to provide debugging, flash programming, and serial
communication over USB.

To use the Segger J-Link tools with OpenSDA, follow the instructions in the
:ref:`nxp_opensda_jlink` page using the `Segger J-Link OpenSDA V2.1 Firmware`_.
The Segger J-Link tools are the default for this board, therefore it is not
necessary to set ``OPENSDA_FW=jlink`` explicitly when you invoke ``make
debug``.

With these mechanisms, applications for the ``mimxrt1020_evk`` board
configuration can be built and debugged in the usual way (see
:ref:`build_an_application` and :ref:`application_run` for more details).

The pyOCD tools do not yet support this SoC.

Flashing
========

The Segger J-Link firmware does not support command line flashing, therefore
the usual ``flash`` build system target is not supported.
Instead, see the https://www.nxp.com/docs/en/application-note/AN12108.pdf for flashing instructions.


Debugging
=========

This example uses the :ref:`hello_world` sample with the
:ref:`nxp_opensda_jlink` tools. Run the following to build your Zephyr
application, invoke the J-Link GDB server, attach a GDB client, and program
your Zephyr application to flash. It will leave you at a GDB prompt.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mimxrt1020_evk
   :goals: debug


.. _MIMXRT1020-EVK Website:
   https://www.nxp.com/support/developer-resources/run-time-software/i.mx-developer-resources/i.mx-rt1020-evaluation-kit:MIMXRT1020-EVK

.. _MIMXRT1020-EVK User Guide:
   https://www.nxp.com/docs/en/user-guide/MIMXRT1020EVKHUG.pdf

.. _MIMXRT1020-EVK Design Files:
   https://www.nxp.com/webapp/Download?colCode=MIMXRT1020-EVK-Design-Files

.. _i.MX RT1020 Website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-based-processors-and-mcus/i.mx-applications-processors/i.mx-rt-series/i.mx-rt1020-crossover-processor-with-arm-cortex-m7-core:i.MX-RT1020

.. _i.MX RT1020 Datasheet:
   https://www.nxp.com/docs/en/data-sheet/IMXRT1020CEC.pdf

.. _i.MX RT1020 Reference Manual:
   https://www.nxp.com/webapp/Download?colCode=IMXRT1020RM

.. _Segger J-Link OpenSDA V2.1 Firmware:
   https://www.segger.com/downloads/jlink/OpenSDA_V2_1.bin
