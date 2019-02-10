.. _mimxrt1064_evk:

NXP MIMXRT1064-EVK
##################

Overview
********

The i.MX RT1064 is the latest addition to the industry's first crossover
processor series and expands the i.MX RT series to three scalable families.
The i.MX RT1064 doubles the On-Chip SRAM to 1MB while keeping pin-to-pin
compatibility with i.MX RT1050. This new series introduces additional features
ideal for real-time applications such as High-Speed GPIO, CAN-FD, and
synchronous parallel NAND/NOR/PSRAM controller. The i.MX RT1064 runs on the
Arm® Cortex-M7® core at 600 MHz.

.. image:: ./mimxrt1064_evk.jpg
   :width: 720px
   :align: center
   :alt: MIMXRT1064-EVK

Hardware
********

- MIMXRT1064DVL6A MCU (600 MHz, 1024 KB on-chip memory, 4096KB on-chip QSPI
  flash)

- Memory

  - 256 Mbit SDRAM
  - 64 Mbit QSPI Flash
  - 512 Mbit Hyper Flash
  - TF socket for SD card

- Display

  - LCD connector

- Ethernet

  - 10/100 Mbit/s Ethernet PHY

- USB

  - USB 2.0 OTG connector
  - USB 2.0 host connector

- Audio

  - 3.5 mm audio stereo headphone jack
  - Board-mounted microphone
  - Left and right speaker out connectors

- Power

  - 5 V DC jack

- Debug

  - JTAG 20-pin connector
  - OpenSDA with DAPLink

- Sensor

  - FXOS8700CQ 6-axis e-compass
  - CMOS camera sensor interface

- Expansion port

  - Arduino interface

- CAN bus connector

For more information about the MIMXRT1064 SoC and MIMXRT1064-EVK board, see
these references:

- `i.MX RT1064 Website`_
- `i.MX RT1064 Datasheet`_
- `i.MX RT1064 Reference Manual`_
- `MIMXRT1064-EVK Website`_
- `MIMXRT1064-EVK Quick Reference Guide`_
- `MIMXRT1064-EVK Schematics`_

Supported Features
==================

The mimxrt1064_evk board configuration supports the following hardware
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
``boards/arm/mimxrt1064_evk/mimxrt1064_evk_defconfig``

Other hardware features are not currently supported by the port.

Connections and I/Os
====================

The MIMXRT1064 SoC has four pairs of pinmux/gpio controllers.

+---------------+-----------------+---------------------------+
| Name          | Function        | Usage                     |
+===============+=================+===========================+
| GPIO_AD_B0_09 | GPIO            | LED                       |
+---------------+-----------------+---------------------------+
| GPIO_AD_B0_12 | LPUART1_TX      | UART Console              |
+---------------+-----------------+---------------------------+
| GPIO_AD_B0_13 | LPUART1_RX      | UART Console              |
+---------------+-----------------+---------------------------+
| WAKEUP        | GPIO            | SW0                       |
+---------------+-----------------+---------------------------+

System Clock
============

The MIMXRT1064 SoC is configured to use the 24 MHz external oscillator on the
board with the on-chip PLL to generate a 600 MHz core clock.

Serial Port
===========

The MIMXRT1064 SoC has eight UARTs. One is configured for the console and the
remaining are not used.

Programming and Debugging
*************************

The MIMXRT1064-EVK includes the :ref:`nxp_opensda` serial and debug adapter
built into the board to provide debugging, flash programming, and serial
communication over USB.

To use the Segger J-Link tools with OpenSDA, follow the instructions in the
:ref:`nxp_opensda_jlink` page using the `Segger J-Link OpenSDA V2.1 Firmware`_.
The Segger J-Link tools are the default for this board, therefore it is not
necessary to set ``OPENSDA_FW=jlink`` explicitly when you invoke ``make
debug``.

With these mechanisms, applications for the ``mimxrt1064_evk`` board
configuration can be built and debugged in the usual way (see
:ref:`build_an_application` and :ref:`application_run` for more details).

The pyOCD tools do not yet support this SoC.

Flashing
========

The Segger J-Link firmware does not support command line flashing, therefore
the usual ``flash`` build system target is not supported.
Instead, see the NXP `How to Enable Boot from QSPI Flash App Note
<https://www.nxp.com/docs/en/application-note/AN12108.pdf>`_ for flashing instructions.


Debugging
=========

This example uses the :ref:`hello_world` sample with the
:ref:`nxp_opensda_jlink` tools. Run the following to build your Zephyr
application, invoke the J-Link GDB server, attach a GDB client, and program
your Zephyr application to flash. It will leave you at a GDB prompt.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mimxrt1064_evk
   :goals: debug


.. _MIMXRT1064-EVK Website:
   https://www.nxp.com/support/developer-resources/run-time-software/i.mx-developer-resources/mimxrt1064-evk-i.mx-rt1064-evaluation-kit:MIMXRT1064-EVK

.. _MIMXRT1064-EVK Quick Reference Guide:
   https://www.nxp.com/docs/en/quick-reference-guide/IMXRT1064QSG.pdf

.. _MIMXRT1064-EVK Schematics:
   https://www.nxp.com/webapp/Download?colCode=i.MXRT160EVKDS&Parent_nodeId=1537930933174731284155&Parent_pageType=product

.. _i.MX RT1064 Website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-based-processors-and-mcus/i.mx-applications-processors/i.mx-rt-series/i.mx-rt1064-crossover-processor-with-arm-cortex-m7-core:i.MX-RT1064

.. _i.MX RT1064 Datasheet:
   https://www.nxp.com/docs/en/data-sheet/IMXRT1064CEC.pdf

.. _i.MX RT1064 Reference Manual:
   https://www.nxp.com/docs/en/reference-manual/IMXRT1064RM.pdf

.. _Segger J-Link OpenSDA V2.1 Firmware:
   https://www.segger.com/downloads/jlink/OpenSDA_V2_1.bin
