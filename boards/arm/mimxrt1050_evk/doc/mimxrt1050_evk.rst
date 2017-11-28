.. _mimxrt1050_evk:

NXP MIMXRT1050-EVK
##################

Overview
********

The i.MX RT1050 is a new processor family featuring NXP's advanced
implementation of the ARM Cortex-M7 Core. It provides high CPU performance and
real-time response. The i.MX RT1050 provides various memory interfaces,
including SDRAM, Raw NAND FLASH, NOR FLASH, SD/eMMC, Quad SPI, HyperBus and a
wide range of other interfaces for connecting peripherals, such as WLAN,
Bluetooth™, GPS, displays, and camera sensors. As with other i.MX processors,
i.MX RT1050 also has rich audio and video features, including LCD display,
basic 2D graphics, camera interface, SPDIF, and I2S audio interface.

.. image:: mimxrt1050_evk.jpg
   :width: 720px
   :align: center
   :alt: MIMXRT1050-EVK

Hardware
********

- MIMXRT1052DVL6A MCU (600 MHz, 512 KB TCM)

- Memory

  - 256 KB SDRAM
  - 64 Mbit QSPI Flash
  - 512 Mbit Hyper Flash

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

For more information about the MIMXRT1050 SoC and MIMXRT1050-EVK board, see
these references:

- `i.MX RT1050 Website`_
- `i.MX RT1050 Datasheet`_
- `i.MX RT1050 Reference Manual`_
- `MIMXRT1050-EVK Website`_
- `MIMXRT1050-EVK User Guide`_
- `MIMXRT1050-EVK Schematics`_

Supported Features
==================

The mimxrt1050_evk board configuration supports the following hardware
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

	``boards/arm/mimxrt1050_evk/mimxrt1050_evk_defconfig``

Other hardware features are not currently supported by the port.

Connections and IOs
===================

The MIMXRT1050 SoC has five pairs of pinmux/gpio controllers.

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

The MIMXRT1050 SoC is configured to use the 24 MHz external oscillator on the
board with the on-chip PLL to generate a 600 MHz core clock.

Serial Port
===========

The MIMXRT1050 SoC has eight UARTs. One is configured for the console and the
remaining are not used.

Programming and Debugging
*************************

The MIMXRT1050-EVK includes the :ref:`nxp_opensda` serial and debug adapter
built into the board to provide debugging, flash programming, and serial
communication over USB.

To use the Segger J-Link tools with OpenSDA, follow the instructions in the
:ref:`nxp_opensda_jlink` page using the `Segger J-Link OpenSDA V2.1 Firmware`_.
The Segger J-Link tools are the default for this board, therefore it is not
necessary to set ``OPENSDA_FW=jlink`` explicitly when you invoke ``make
debug``.

With these mechanisms, applications for the ``mimxrt1050_evk`` board
configuration can be built and debugged in the usual way (see
:ref:`build_an_application` and :ref:`application_run` for more details).

The pyOCD tools do not yet support this SoC.

Flashing
========

The Segger J-Link firmware does not support command line flashing, therefore
the usual ``flash`` build system target is not supported.

Debugging
=========

This example uses the :ref:`hello_world` sample with the
:ref:`nxp_opensda_jlink` tools. Run the following to build your Zephyr
application, invoke the J-Link GDB server, attach a GDB client, and program
your Zephyr application to flash. It will leave you at a GDB prompt.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mimxrt1050_evk
   :goals: debug


.. _MIMXRT1050-EVK Website:
   https://www.nxp.com/products/microcontrollers-and-processors/arm-based-processors-and-mcus/i.mx-applications-processors/i.mx-rt-series/i.mx-rt1050-evaluation-kit:MIMXRT1050-EVK

.. _MIMXRT1050-EVK User Guide:
   https://www.nxp.com/docs/en/user-guide/MIMXRT1050EVKHUG.pdf

.. _MIMXRT1050-EVK Schematics:
   https://www.nxp.com/webapp/Download?colCode=MIMXRT1050-EVK-DESIGNFILES

.. _i.MX RT1050 Website:
   https://www.nxp.com/products/microcontrollers-and-processors/arm-based-processors-and-mcus/i.mx-applications-processors/i.mx-rt-series/i.mx-rt1050-crossover-processor-with-arm-cortex-m7-core:i.MX-RT1050

.. _i.MX RT1050 Datasheet:
   https://www.nxp.com/docs/en/data-sheet/IMXRT1050CEC.pdf

.. _i.MX RT1050 Reference Manual:
   https://www.nxp.com/docs/en/reference-manual/IMXRT1050RM.pdf

.. _DAPLink FRDM-K64F Firmware:
   http://www.nxp.com/assets/downloads/data/en/ide-debug-compile-build-tools/OpenSDAv2.2_DAPLink_frdmk64f_rev0242.zip

.. _Segger J-Link OpenSDA V2.1 Firmware:
   https://www.segger.com/downloads/jlink/OpenSDA_V2_1.bin
