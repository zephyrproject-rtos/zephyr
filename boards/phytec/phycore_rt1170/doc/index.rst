.. _phycore_rt1170:

phyCORE-RT1170 (MIMXRT1176)
###########################

Overview
********

PHYTEC phyCORE-RT1170 board is based on the NXP dual core i.MX RT1170 which
runs on the Cortex-M7 core at 1 GHz and on the Cortex-M4 at 400 MHz.
Zephyr OS is ported to run on this board with initial support of some peripherals.

.. image:: img/phycore_pcm073.webp
   :align: center
   :alt: phyCORE-RT1170

Hardware
********

- MIMXRT1176DVMAA MCU

  - 1GHz Cortex-M7 & 400Mhz Cortex-M4
  - 2MB SRAM with 512KB of TCM for Cortex-M7 and 256KB of TCM for Cortex-M4

- Memory

  - 512 Mbit SDRAM
  - 128 Mbit QSPI Flash
  - 512 Mbit Octal Flash

For more information about phyCORE-RT1170 board, see
these references:

- `PHYTEC Website`_

External Memory
===============

This platform has the following external memories:

+----------------------+------------+-------------------------------------+
| Device               | Controller | Status                              |
+======================+============+=====================================+
| MT48LC16M16A2B4-7EIT | SEMC       | Enabled via device configuration    |
| SDRAM                |            | data (DCD) block, which sets up     |
|                      |            | the SEMC at boot time               |
+----------------------+------------+-------------------------------------+
| MX25U12832FM2I02     | FLEXSPI    | Enabled via flash configuration     |
| QSPI flash           |            | block (FCB), which sets up the      |
|                      |            | FLEXSPI at boot time.               |
+----------------------+------------+-------------------------------------+

Supported Features
==================

The PHYTEC phyCORE-RT1170 configuration supports the following hardware
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
| SPI       | on-chip    | spi                                 |
+-----------+------------+-------------------------------------+
| I2C       | on-chip    | i2c                                 |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt;              |
|           |            | serial port-async                   |
+-----------+------------+-------------------------------------+
| FLEXSPI   | on-chip    | flash programming                   |
+-----------+------------+-------------------------------------+

The default configuration can be found in the defconfig files:
:zephyr_file:`boards/phytec/phycore_rt1170/phycore_rt1170_mimxrt1176_cm7_defconfig`
:zephyr_file:`boards/phytec/phycore_rt1170/phycore_rt1170_mimxrt1176_cm4_defconfig`

Connections and I/Os
====================

The MIMXRT1170 SoC has six pairs of pinmux/gpio controllers.

+---------------------------+----------------+------------------+
| Name                      | Function       | Usage            |
+---------------------------+----------------+------------------+
| GPIO_AD_24                | LPUART1_TX     | UART Console     |
+---------------------------+----------------+------------------+
| GPIO_AD_25                | LPUART1_RX     | UART Console     |
+---------------------------+----------------+------------------+
| GPIO_AD_28                | LPUART5_TX     | UART Console     |
+---------------------------+----------------+------------------+
| GPIO_AD_29                | LPUART5_RX     | UART Console     |
+---------------------------+----------------+------------------+
| GPIO_EMC_B1_40            | LPUART6_TX     | UART Console     |
+---------------------------+----------------+------------------+
| GPIO_EMC_B1_41            | LPUART6_RX     | UART Console     |
+---------------------------+----------------+------------------+
| GPIO_AD_29                | SPI1_CS0       | spi              |
+---------------------------+----------------+------------------+
| GPIO_AD_28                | SPI1_CLK       | spi              |
+---------------------------+----------------+------------------+
| GPIO_AD_30                | SPI1_SDO       | spi              |
+---------------------------+----------------+------------------+
| GPIO_AD_31                | SPI1_SDI       | spi              |
+---------------------------+----------------+------------------+
| GPIO_AD_08                | LPI2C1_SCL     | i2c              |
+---------------------------+----------------+------------------+
| GPIO_AD_09                | LPI2C1_SDA     | i2c              |
+---------------------------+----------------+------------------+
| GPIO_AD_18                | LPI2C2_SCL     | i2c              |
+---------------------------+----------------+------------------+
| GPIO_AD_19                | LPI2C2_SDA     | i2c              |
+---------------------------+----------------+------------------+

Dual Core samples
*****************

+-----------+------------------+----------------------------+
| Core      | Boot Address     | Comment                    |
+===========+==================+============================+
| Cortex M7 | 0x30000000[630K] | primary core               |
+-----------+------------------+----------------------------+
| Cortex M4 | 0x20020000[96k]  | boots from OCRAM           |
+-----------+------------------+----------------------------+

+----------+------------------+-----------------------+
| Memory   | Address[Size]    | Comment               |
+==========+==================+=======================+
| flexspi1 | 0x30000000[16M]  | Cortex M7 flash       |
+----------+------------------+-----------------------+
| sdram0   | 0x80030000[64M]  | Cortex M7 ram         |
+----------+------------------+-----------------------+
| ocram    | 0x20020000[512K] | Cortex M4 "flash"     |
+----------+------------------+-----------------------+
| sram1    | 0x20000000[128K] | Cortex M4 ram         |
+----------+------------------+-----------------------+
| ocram2   | 0x200C0000[512K] | Mailbox/shared memory |
+----------+------------------+-----------------------+

Only the first 16K of ocram2 has the correct MPU region attributes set to be
used as shared memory

System Clock
============

The MIMXRT1170 SoC is configured to use SysTick as the system clock source,
running at 996MHz. When targeting the M4 core, SysTick will also be used,
running at 400MHz

When power management is enabled, the 32 KHz low frequency
oscillator on the board will be used as a source for the GPT timer to
generate a system clock. This clock enables lower power states, at the
cost of reduced resolution

Serial Port
===========

The MIMXRT1170 SoC has 12 UARTs. One is configured for the console and the
remaining are not used.

Programming and Debugging
*************************

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Configuring a Debug Probe
=========================

A debug probe is used for both flashing and debugging the board. The on-board
debugger works with the JLink runner by default.

Using J-Link
------------

JLink is the default runner for this board.  Install the
:ref:`jlink-debug-host-tools` and make sure they are in your search path.


Configuring a Console
=====================

We will use the on-board debugger
microcontroller as a usb-to-serial adapter for the serial console.

Use the following settings with your serial terminal of choice (minicom, putty,
etc.):

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flashing
========

Here is an example for the :ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: phycore_rt1170/mimxrt1176/cm7
   :goals: flash

Power off the board. Then power on the board and
open a serial terminal, reset the board and you should
see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v3.6.0-xxxx-xxxxxxxxxxxxx *****
   Hello World! phycore_rt1170/mimxrt1176/cm7

Debugging
=========

Here is an example for the :ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: phycore_rt1170/mimxrt1176/cm7
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v3.6.0-xxxx-xxxxxxxxxxxxx *****
   Hello World! phycore_rt1170/mimxrt1176/cm7

.. _PHYTEC Website:
   https://www.phytec.com/product/phycore-rt1170

.. _AN13264:
   https://www.nxp.com/docs/en/application-note/AN13264.pdf
