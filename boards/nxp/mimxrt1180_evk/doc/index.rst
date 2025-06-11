.. zephyr:board:: mimxrt1180_evk

Overview
********

The dual core i.MX RT1180 runs on the Cortex-M33 core at 240 MHz and on the
Cortex-M7 at 792 MHz. The i.MX RT1180 MCU offers support over a wide
temperature range and is qualified for consumer, industrial and automotive
markets.

Hardware
********

- MIMXRT1189CVM8B MCU

  - 240MHz Cortex-M33 with 256KB TCM and 16 KB caches
  - 792Mhz Cortex-M7 with 512KB TCM and 32 KB caches
  - 1.5MB SRAM

- Memory

  - 512 Mbit SDRAM
  - 128 Mbit QSPI Flash
  - 512 Mbit HYPER RAM
  - TF socket for SD card

- Ethernet

  - 1000 Mbit/s Ethernet PHY

- USB

  - 2* USB 2.0 OTG connector

- Audio

  - 3.5 mm audio stereo headphone jack
  - Board-mounted microphone
  - Left and right speaker out connectors

- Power

  - 5 V DC jack

- Debug

  - JTAG 20-pin connector
  - MCU-Link with DAPLink

- Expansion port

  - Arduino interface

- CAN bus connector

For more information about the MIMXRT1180 SoC and MIMXRT1180-EVK board, see
these references:

- `i.MX RT1180 Website`_
- `MIMXRT1180-EVK Website`_

External Memory
===============

This platform has the following external memories:

+--------------------+------------+-------------------------------------+
| Device             | Controller | Status                              |
+====================+============+=====================================+
| W9825G6KH          | SEMC       | Enabled via device configuration    |
|                    |            | data block, which sets up SEMC at   |
|                    |            | boot time                           |
+--------------------+------------+-------------------------------------+
| W25Q128JWSIQ       | FLEXSPI    | Enabled via flash configuration     |
|                    |            | block, which sets up FLEXSPI at     |
|                    |            | boot time.                          |
+--------------------+------------+-------------------------------------+

Supported Features
==================

NXP considers the MIMXRT1180-EVK as the superset board for the i.MX RT118x
family of MCUs.  This board is a focus for NXP's Full Platform Support for
Zephyr, to better enable the entire RT118x family.  NXP prioritizes enabling
this board with new support for Zephyr features.

.. zephyr:board-supported-hw::

Connections and I/Os
====================

The MIMXRT1180 SoC has six pairs of pinmux/gpio controllers.

+---------------+-----------------+---------------------------+
| Name          | Function        | Usage                     |
+===============+=================+===========================+
| GPIO_AON_04   | GPIO            | SW8                       |
+---------------+-----------------+---------------------------+
| GPIO_AD_27    | GPIO            | LED                       |
+---------------+-----------------+---------------------------+
| GPIO_AON_08   | LPUART1_TX      | UART Console M33 core     |
+---------------+-----------------+---------------------------+
| GPIO_AON_09   | LPUART1_RX      | UART Console M33 core     |
+---------------+---------------------------------------------+
| GPIO_AON_19   | LPUART12_TX     | UART Console M7 core      |
+---------------+-----------------+---------------------------+
| GPIO_AON_20   | LPUART12_RX     | UART Console M7 core      |
+---------------+-----------------+---------------------------+
| GPIO_SD_B1_00 | SPI1_CS0        | spi                       |
+---------------+---------------------------------------------+
| GPIO_SD_B1_01 | SPI1_CLK        | spi                       |
+---------------+---------------------------------------------+
| GPIO_SD_B1_02 | SPI1_SDO        | spi                       |
+---------------+---------------------------------------------+
| GPIO_SD_B1_03 | SPI1_SDI        | spi                       |
+---------------+-----------------+---------------------------+

UART for M7 core is connected to USB-to-UART J60 connector.
Or user can use open JP7 Jumper to enable second UART on MCU LINK J53 connector.

System Clock
============

The MIMXRT1180 SoC is configured to use SysTick as the system clock source,
running at 240MHz. When targeting the M7 core, SysTick will also be used,
running at 792MHz

Serial Port
===========

The MIMXRT1180 SoC has 12 UARTs. LPUART1 is configured for the CM33 console, the LPUART12 is
configured for the CM7 console core and the remaining are not used.

Ethernet
========

NETC Ethernet driver supports to manage the Physical Station Interface (PSI).
NETC DSA driver supports to manage switch ports. Current DSA support is with
limitation that only switch function is available without management via
DSA master port. DSA master port support is TODO work.

.. code-block:: none

                   +--------+                  +--------+
                   | ENETC1 |                  | ENETC0 |
                   |        |                  |        |
                   | Pseudo |                  |  1G    |
                   |  MAC   |                  |  MAC   |
                   +--------+                  +--------+
                       | zero copy interface       |
   +-------------- +--------+----------------+     |
   |               | Pseudo |                |     |
   |               |  MAC   |                |     |
   |               |        |                |     |
   |               | Port 4 |                |     |
   |               +--------+                |     |
   |           SWITCH       CORE             |     |
   +--------+ +--------+ +--------+ +--------+     |
   | Port 0 | | Port 1 | | Port 2 | | Port 3 |     |
   |        | |        | |        | |        |     |
   |  1G    | |  1G    | |  1G    | |  1G    |     |
   |  MAC   | |  MAC   | |  MAC   | |  MAC   |     |
   +--------+-+--------+-+--------+-+--------+     |
       |          |          |          |          |
   NETC External Interfaces (4 switch ports, 1 end-point port)

Dual Core Operation
*******************

The MIMXRT1180 EVK supports dual core operation with both the Cortex-M33 and Cortex-M7 cores.
By default, the CM33 core is the boot core and is responsible for initializing the system and
starting the CM7 core.

CM7 Execution Modes
===================

The CM7 core can execute code in two different modes:

1. **ITCM Mode (Default)**: The CM7 code is copied from flash to ITCM (Instruction Tightly Coupled Memory)
   and executed from there. This provides faster execution but is limited by the ITCM size.

2. **Flash Mode**: The CM7 code is executed directly from flash memory (XIP - eXecute In Place).
   This allows for larger code size but may be slower than ITCM execution.
   When booting CM7 from Flash the TRDC execution permissions has to be set by CM33 core.

Configuring CM7 Execution Mode
==============================

To configure the CM7 execution mode, you can use the following Kconfig option:

.. code-block:: none

   CONFIG_CM7_BOOT_FROM_FLASH=n  # For ITCM execution (default)
   CONFIG_CM7_BOOT_FROM_FLASH=y  # For flash execution

When building with west, you can specify this option on the command line:

.. code-block:: bash

   # For ITCM execution (default)
   west build -b mimxrt1180_evk/mimxrt1189/cm33 samples/drivers/mbox --sysbuild

   # For flash execution
   west build -b mimxrt1180_evk/mimxrt1189/cm33 <sample_path> --sysbuild -- \
     -D<remote_app_name>_EXTRA_DTC_OVERLAY_FILE=${ZEPHYR_BASE}/boards/nxp/mimxrt1180_evk/cm7_flash_boot.overlay \
     -DCONFIG_CM7_BOOT_FROM_FLASH=y -D<remote_app_name>_CONFIG_CM7_BOOT_FROM_FLASH=y

  west build -b mimxrt1180_evk/mimxrt1189/cm33 samples/drivers/mbox --sysbuild -- \
     -Dremote_EXTRA_DTC_OVERLAY_FILE=${ZEPHYR_BASE}/boards/nxp/mimxrt1180_evk/cm7_flash_boot.overlay \
     -DCONFIG_CM7_BOOT_FROM_FLASH=y -Dremote_CONFIG_CM7_BOOT_FROM_FLASH=y

Flash Boot Overlay
==================

When executing the CM7 core from flash, you need to apply a device tree overlay to configure
the flash memory properly. The overlay file is located at:

.. code-block:: none

   boards/nxp/mimxrt1180_evk/cm7_flash_boot.overlay

This overlay configures the CM7 core to use the flash memory for code execution instead of ITCM.

Memory Usage
============

* **ITCM Mode**: The CM7 code is copied from flash to ITCM.
* **Flash Mode**: The CM7 code is executed directly from flash, which allows for larger code size.

Performance Considerations
==========================

* **ITCM Mode**: Provides faster execution due to the low-latency ITCM memory.
* **Flash Mode**: May be slower due to flash memory access times, but allows for larger code size.

Dual Core samples Debugging
===========================

When debugging dual core samples, need to ensure the SW5 on "0100" mode.
The CM33 core is responsible for copying and starting the CM7.
To debug the CM7 it is useful to put infinite while loop either in reset vector or
into main function and attach via debugger to CM7 core.

CM7 core can be started again only after reset, so after flashing ensure to reset board.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Configuring a Debug Probe
=========================

LinkServer is the default runner for this board.
A debug probe is used for both flashing and debugging the board. This board is
configured by default to use the :ref:`mcu-link-cmsis-onboard-debug-probe`.
The :ref:`pyocd-debug-host-tools` do not yet support programming the
external flashes on this board. Use one of the other supported debug probes
below.

.. _Using J-Link RT1180:

Using J-Link
------------

Please ensure to use a version of JLINK above V7.94g and jumper JP5 is installed if using
external jlink plus on J37 as debugger.

When debugging cm33 core, need to ensure the SW5 on "0100" mode.
When debugging cm7 core, need to ensure the SW5 on "0001" mode.
(Only support run cm7 image when debugging due to default boot core on board is cm33 core)

Install the :ref:`jlink-debug-host-tools` and make sure they are in your search
path.

There are two options: the onboard debug circuit can be updated with Segger
J-Link firmware, or :ref:`jlink-external-debug-probe` can be attached to the
EVK.


Using Linkserver
----------------

Please ensure to use a version of Linkserver above V1.5.30 and jumper JP5 is uninstalled (default setting).

When debugging cm33 core, need to ensure the SW5 on "0100" mode.
When debugging cm7 core, need to ensure the SW5 on "0001" mode.
(Only support run cm7 image when debugging due to default boot core on board is cm33 core)

Configuring a Console
=====================

Regardless of your choice in debug probe, we will use the MCU-Link
microcontroller as a usb-to-serial adapter for the serial console. Check that
jumpers JP5 and JP3 are **on** (they are on by default when boards ship from
the factory) to connect UART signals to the MCU-Link microcontroller.

Connect a USB cable from your PC to J53.

Use the following settings with your serial terminal of choice (minicom, putty,
etc.):

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flashing
========

Here is an example for the :zephyr:code-sample:`hello_world` application on cm33 core.

Before power on the board, make sure SW5 is set to 0100b

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mimxrt1180_evk/mimxrt1189/cm33
   :goals: flash

Power off the board, then power on the board and
open a serial terminal, reset the board (press the SW3 button), and you should
see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v3.7.0-xxx-xxxxxxxxxxxxx *****
   Hello World! mimxrt1180_evk/mimxrt1189/cm33

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mimxrt1180_evk/mimxrt1189/cm33
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v3.7.0-xxx-xxxxxxxxxxxxx *****
   Hello World! mimxrt1180_evk/mimxrt1189/cm33

.. include:: ../../common/board-footer.rst
   :start-after: nxp-board-footer

.. _MIMXRT1180-EVK Website:
   https://www.nxp.com/design/design-center/development-boards-and-designs/i-mx-evaluation-and-development-boards/i-mx-rt1180-evaluation-kit:MIMXRT1180-EVK

.. _i.MX RT1180 Website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/i-mx-rt-crossover-mcus/i-mx-rt1180-crossover-mcu-with-tsn-switch-and-edgelock:i.MX-RT1180
