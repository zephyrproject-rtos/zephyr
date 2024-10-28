.. zephyr:board:: mimxrt1015_evk

Overview
********

The i.MX RT1015 expands the i.MX RT crossover processor families by providing
high-performance feature set in low-cost LQFP packages, further simplifying
board design and layout for customers. The i.MX RT1015 runs on the Arm®
Cortex®-M7 core at 500 MHz.

Hardware
********

- MIMXRT1015DAF5A MCU

- Memory

  - 128 Mbit QSPI Flash

- Connectivity

  - Micro USB host and OTG connectors
  - Arduino interface

- Audio

  - Audio Codec
  - 4-pole audio headphone jack
  - External speaker connection
  - Microphone

- Debug

  - JTAG 10-pin connector
  - OpenSDA with DAPLink

For more information about the MIMXRT1015 SoC and MIMXRT1015-EVK board, see
these references:

- `i.MX RT1015 Website`_
- `i.MX RT1015 Datasheet`_
- `i.MX RT1015 Reference Manual`_
- `MIMXRT1015-EVK Website`_
- `MIMXRT1015-EVK Quick Reference Guide`_
- `MIMXRT1015-EVK Design Files`_

External Memory
===============

This platform has the following external memories:

+--------------------+------------+-------------------------------------+
| Device             | Controller | Status                              |
+====================+============+=====================================+
| AT25SF128A         | FLEXSPI    | Enabled via flash configurationn    |
|                    |            | block, which sets up FLEXSPI at     |
|                    |            | boot time.                          |
+--------------------+------------+-------------------------------------+

Supported Features
==================

The mimxrt1015_evk board configuration supports the hardware features listed
below.  For additional features not yet supported, please also refer to the
:zephyr:board:`mimxrt1064_evk` , which is the superset board in NXP's i.MX RT10xx family.
NXP prioritizes enabling the superset board with NXP's Full Platform Support for
Zephyr.  Therefore, the mimxrt1064_evk board may have additional features
already supported, which can also be re-used on this mimxrt1015_evk board:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| I2C       | on-chip    | i2c                                 |
+-----------+------------+-------------------------------------+
| SPI       | on-chip    | spi                                 |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+
| USB       | on-chip    | USB device                          |
+-----------+------------+-------------------------------------+
| ADC       | on-chip    | ADC                                 |
+-----------+------------+-------------------------------------+
| GPT       | on-chip    | gpt                                 |
+-----------+------------+-------------------------------------+
| TRNG      | on-chip    | entropy                             |
+-----------+------------+-------------------------------------+

The default configuration can be found in
:zephyr_file:`boards/nxp/mimxrt1015_evk/mimxrt1015_evk_defconfig`

Other hardware features are not currently supported by the port.

Connections and I/Os
====================

The MIMXRT1015 SoC has five pairs of pinmux/gpio controllers.

+---------------+-----------------+---------------------------+
| Name          | Function        | Usage                     |
+===============+=================+===========================+
| GPIO_AD_B0_05 | GPIO            | LED                       |
+---------------+-----------------+---------------------------+
| GPIO_EMC_09   | GPIO            | SW4                       |
+---------------+-----------------+---------------------------+
| GPIO_AD_B0_06 | LPUART1_TX      | UART Console              |
+---------------+-----------------+---------------------------+
| GPIO_AD_B0_07 | LPUART1_RX      | UART Console              |
+---------------+-----------------+---------------------------+
| GPIO_EMC_32   | LPUART4_TX      | UART Console              |
+---------------+-----------------+---------------------------+
| GPIO_EMC_33   | LPUART4_RX      | UART Console              |
+---------------+-----------------+---------------------------+
| GPIO_AD_B1_15 | LPI2C1_SDA      | I2C SDA                   |
+---------------+-----------------+---------------------------+
| GPIO_AD_B1_14 | LPI2C1_CLK      | I2C SCL                   |
+---------------+-----------------+---------------------------+
| GPIO_AD_B0_10 | LPSPI1_SCK      | SPI                       |
+---------------+-----------------+---------------------------+
| GPIO_AD_B0_11 | LPSPI1_PCS0     | SPI                       |
+---------------+-----------------+---------------------------+
| GPIO_AD_B0_12 | LPSPI1_SDO      | SPI                       |
+---------------+-----------------+---------------------------+
| GPIO_AD_B0_13 | LPSPI1_SDI      | SPI                       |
+---------------+-----------------+---------------------------+
| GPIO_AD_B0_14 | ADC             | ADC1 Channel 1            |
+---------------+-----------------+---------------------------+
| GPIO_AD_B1_13 | ADC             | ADC1 Channel 13           |
+---------------+-----------------+---------------------------+

System Clock
============

The MIMXRT1015 SoC is configured to use SysTick as the system clock source,
running at 500MHz.

When power management is enabled, the 32 KHz low frequency
oscillator on the board will be used as a source for the GPT timer to
generate a system clock. This clock enables lower power states, at the
cost of reduced resolution

Serial Port
===========

The MIMXRT1015 SoC has four UARTs. ``LPUART1`` is configured for the console,
and the remaining are not used.

Programming and Debugging
*************************

This board supports 3 debug host tools. Please install your preferred host
tool, then follow the instructions in `Configuring a Debug Probe`_ to
configure the board appropriately.

* :ref:`linkserver-debug-host-tools` (Default, Supported by NXP)
* :ref:`jlink-debug-host-tools` (Supported by NXP)
* :ref:`pyocd-debug-host-tools` (Not supported by NXP)

Once the host tool and board are configured, build and flash applications
as usual (see :ref:`build_an_application` and :ref:`application_run` for more
details).

Configuring a Debug Probe
=========================

For the RT1015, J47/J48 are the SWD isolation jumpers, J42 is the DFU
mode jumper, and J34 is the 10 pin JTAG/SWD header.

.. include:: ../../common/rt1xxx-lpclink2-debug.rst
   :start-after: rt1xxx-lpclink2-probes

Configuring a Console
=====================

Regardless of your choice in debug probe, we will use the OpenSDA
microcontroller as a usb-to-serial adapter for the serial console. Check that
jumpers J45 and J46 are **on** (they are on by default when boards ship from
the factory) to connect UART signals to the OpenSDA microcontroller.

Connect a USB cable from your PC to J41.

Use the following settings with your serial terminal of choice (minicom, putty,
etc.):

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flashing
========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
    :zephyr-app: samples/hello_world
    :board: mimxrt1015_evk
    :goals: flash

Open a serial terminal, reset the board (press the SW9 button), and you should
see the following message in the terminal:

.. code-block:: console

    ***** Booting Zephyr OS v1.14.0-rc1-1297-g312d75f2459e *****
    Hello World! mimxrt1015_evk


.. _MIMXRT1015-EVK Website:
   https://www.nxp.com/support/developer-resources/run-time-software/i.mx-developer-resources/i.mx-rt1015-evaluation-kit:MIMXRT1015-EVK

.. _MIMXRT1015-EVK Quick Reference Guide:
   https://www.nxp.com/webapp/Download?colCode=IMXRT1015QSG

.. _MIMXRT1015-EVK Design Files:
   https://www.nxp.com/webapp/Download?colCode=MIMXRT1015-EVK-REVB-DS

.. _i.MX RT1015 Website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-based-processors-and-mcus/i.mx-applications-processors/i.mx-rt-series/i.mx-rt1015-crossover-processor-with-arm-cortex-m7-core:i.MX-RT1015

.. _i.MX RT1015 Datasheet:
   https://www.nxp.com/docs/en/data-sheet/IMXRT1015CEC.pdf

.. _i.MX RT1015 Reference Manual:
   https://www.nxp.com/webapp/Download?colCode=IMXRT1015RM
