.. zephyr:board:: mimxrt1170_evk

Overview
********

The dual core i.MX RT1170 runs on the Cortex-M7 core at 1 GHz and on the Cortex-M4
at 400 MHz. The i.MX RT1170 MCU offers support over a wide temperature range
and is qualified for consumer, industrial and automotive markets. Zephyr
supports the initial revision of this EVK, as well as rev EVKB.

Hardware
********

- MIMXRT1176DVMAA MCU

  - 1GHz Cortex-M7 & 400Mhz Cortex-M4
  - 2MB SRAM with 512KB of TCM for Cortex-M7 and 256KB of TCM for Cortex-M4

- Memory

  - 512 Mbit SDRAM
  - 128 Mbit QSPI Flash
  - 512 Mbit Octal Flash
  - 2 Gbit raw NAND flash
  - 64 Mbit LPSPI flash
  - TF socket for SD card

- Display

  - MIPI LCD connector

- Ethernet

  - 10/100 Mbit/s Ethernet PHY
  - 10/100/1000 Mbit/s Ethernet PHY

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
  - on-board debugger

- Sensor

  - FXOS8700CQ 6-axis e-compass
  - MIPI camera sensor connector

- Expansion port

  - Arduino interface
  - M.2 WIFI/BT interface

- CAN bus connector

For more information about the MIMXRT1170 SoC and MIMXRT1170-EVK board, see
these references:

- `i.MX RT1170 Website`_
- `i.MX RT1170 Datasheet`_
- `i.MX RT1170 Reference Manual`_
- `MIMXRT1170-EVK Website`_
- `MIMXRT1170-EVK Board Hardware User's Guide`_

External Memory
===============

This platform has the following external memories:

+--------------------+------------+-------------------------------------+
| Device             | Controller | Status                              |
+====================+============+=====================================+
| W9825G6KH          | SEMC       | Enabled via device configuration    |
| SDRAM              |            | data (DCD) block, which sets up     |
|                    |            | the SEMC at boot time               |
+--------------------+------------+-------------------------------------+
| IS25WP128          | FLEXSPI    | Enabled via flash configuration     |
| QSPI flash         |            | block (FCB), which sets up the      |
| (RT1170 EVK)       |            | FLEXSPI at boot time.               |
+--------------------+------------+-------------------------------------+
| W25Q512NWEIQ       | FLEXSPI    | Enabled via flash configuration     |
| QSPI flash         |            | block (FCB), which sets up the      |
| (RT1170 EVKB)      |            | FLEXSPI at boot time. Supported for |
|                    |            | XIP only.                           |
+--------------------+------------+-------------------------------------+

Supported Features
==================

NXP considers the MIMXRT1170-EVK as the superset board for the i.MX RT11xx
family of MCUs.  This board is a focus for NXP's Full Platform Support for
Zephyr, to better enable the entire RT11xx family.

.. zephyr:board-supported-hw::

Connections and I/Os
====================

The MIMXRT1170 SoC has six pairs of pinmux/gpio controllers.

+---------------------------+----------------+------------------+
| Name                      | Function       | Usage            |
+---------------------------+----------------+------------------+
| WAKEUP                    | GPIO           | SW7              |
+---------------------------+----------------+------------------+
| GPIO_AD_04                | GPIO           | LED              |
+---------------------------+----------------+------------------+
| GPIO_AD_24                | LPUART1_TX     | UART Console     |
+---------------------------+----------------+------------------+
| GPIO_AD_25                | LPUART1_RX     | UART Console     |
+---------------------------+----------------+------------------+
| GPIO_LPSR_00              | CAN3_TX        | flexcan          |
+---------------------------+----------------+------------------+
| GPIO_LPSR_01              | CAN3_RX        | flexcan          |
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
| GPIO_LPSR_05              | LPI2C5_SCL     | i2c              |
+---------------------------+----------------+------------------+
| GPIO_LPSR_04              | LPI2C5_SDA     | i2c              |
+---------------------------+----------------+------------------+
| GPIO_AD_04                | FLEXPWM1_PWM2  | pwm              |
+---------------------------+----------------+------------------+
| GPIO_AD_32                | ENET_MDC       | Ethernet         |
+---------------------------+----------------+------------------+
| GPIO_AD_33                | ENET_MDIO      | Ethernet         |
+---------------------------+----------------+------------------+
| GPIO_DISP_B2_02           | ENET_TX_DATA00 | Ethernet         |
+---------------------------+----------------+------------------+
| GPIO_DISP_B2_03           | ENET_TX_DATA01 | Ethernet         |
+---------------------------+----------------+------------------+
| GPIO_DISP_B2_04           | ENET_TX_EN     | Ethernet         |
+---------------------------+----------------+------------------+
| GPIO_DISP_B2_05           | ENET_REF_CLK   | Ethernet         |
+---------------------------+----------------+------------------+
| GPIO_DISP_B2_06           | ENET_RX_DATA00 | Ethernet         |
+---------------------------+----------------+------------------+
| GPIO_DISP_B2_07           | ENET_RX_DATA01 | Ethernet         |
+---------------------------+----------------+------------------+
| GPIO_DISP_B2_08           | ENET_RX_EN     | Ethernet         |
+---------------------------+----------------+------------------+
| GPIO_DISP_B2_09           | ENET_RX_ER     | Ethernet         |
+---------------------------+----------------+------------------+
| GPIO_AD_17_SAI1_MCLK      | SAI_MCLK       | SAI              |
+---------------------------+----------------+------------------+
| GPIO_AD_21_SAI1_TX_DATA00 | SAI1_TX_DATA   | SAI              |
+---------------------------+----------------+------------------+
| GPIO_AD_22_SAI1_TX_BCLK   | SAI1_TX_BCLK   | SAI              |
+---------------------------+----------------+------------------+
| GPIO_AD_23_SAI1_TX_SYNC   | SAI1_TX_SYNC   | SAI              |
+---------------------------+----------------+------------------+
| GPIO_AD_17_SAI1_MCLK      | SAI1_MCLK      | SAI              |
+---------------------------+----------------+------------------+
| GPIO_AD_20_SAI1_RX_DATA00 | SAI1_RX_DATA00 | SAI              |
+---------------------------+----------------+------------------+
| GPIO_DISP_B2_10           | LPUART2_TX     | M.2 BT HCI       |
+---------------------------+----------------+------------------+
| GPIO_DISP_B2_11           | LPUART2_RX     | M.2 BT HCI       |
+---------------------------+----------------+------------------+
| GPIO_DISP_B2_12           | LPUART2_CTS_B  | M.2 BT HCI       |
+---------------------------+----------------+------------------+
| GPIO_DISP_B2_13           | LPUART1_RTS_B  | M.2 BT HCI       |
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

The MIMXRT1170 SoC has 12 UARTs. ``LPUART1`` is configured for the console,
``LPUART2`` for the Bluetooth Host Controller Interface (BT HCI), and the
remaining are not used.

Fetch Binary Blobs
==================

The board Bluetooth/WiFi module requires fetching some binary blob files, to do
that run the command:

.. code-block:: console

   west blobs fetch hal_nxp

.. note:: Only Bluetooth functionality is currently supported.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Building a Dual-Core Image
==========================
Dual core samples load the M4 core image from flash into the shared ``ocram``
region. The M7 core then sets the M4 boot address to this region. The only
sample currently enabled for dual core builds is the ``openamp`` sample.
To flash a dual core sample, the M4 image must be flashed first, so that it is
written to flash. Then, the M7 image must be flashed. The openamp sysbuild
sample will do this automatically by setting the image order.

The secondary core can be debugged normally in single core builds
(where the target is ``mimxrt1170_evk/mimxrt1176/cm4``). For dual core builds, the
secondary core should be placed into a loop, then a debugger can be attached
(see `AN13264`_, section 4.2.3 for more information)

Launching Images Targeting M4 Core
==================================
If building targeting the M4 core, the M7 core must first run code to launch
the M4 image, by copying it into the ``ocram`` region then kicking off the M4
core. When building using sysbuild targeting the M4 core, a minimal "launcher"
image will be built and flashed to the M7 core, which loads and kicks off
the M4 core. Therefore when developing an application intended to run
standalone on the M4 core, it is recommended to build with sysbuild, like
so:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mimxrt1170_evk/mimxrt1176/cm4
   :west-args: --sysbuild
   :goals: flash

If desired, this behavior can be disabled by building with
``-DSB_CONFIG_SECOND_CORE_MCUX_LAUNCHER=n``

Configuring a Debug Probe
=========================

A debug probe is used for both flashing and debugging the board. The on-board
debugger listed below works with the LinkServer runner by default, or can be
reprogrammed with JLink firmware.

- MIMXRT1170-EVKB: :ref:`mcu-link-cmsis-onboard-debug-probe`
- MIMXRT1170-EVK:  :ref:`opensda-daplink-onboard-debug-probe`

Using J-Link
------------

JLink is the default runner for this board.  Install the
:ref:`jlink-debug-host-tools` and make sure they are in your search path.

There are two options: the onboard debug circuit can be updated with Segger
J-Link firmware, or :ref:`jlink-external-debug-probe` can be attached to the
EVK. See `Using J-Link with MIMXRT1170-EVKB`_ or
`Using J-Link with MIMXRT1160-EVK or MIMXRT1170-EVK`_ for more details.

Using LinkServer
----------------

Install the :ref:`linkserver-debug-host-tools` and make sure they are in your
search path.  LinkServer works with the default CMSIS-DAP firmware included in
the on-board debugger.

Use the ``-r linkserver`` option with West to use the LinkServer runner.

.. code-block:: console

   west flash -r linkserver

Alternatively, pyOCD can be used to flash and debug the board by using the
``-r pyocd`` option with West. pyOCD is installed when you complete the
:ref:`gs_python_deps` step in the Getting Started Guide. The runners supported
by NXP are LinkServer and JLink. pyOCD is another potential option, but NXP
does not test or support the pyOCD runner.

Configuring a Console
=====================

We will use the on-board debugger
microcontroller as a usb-to-serial adapter for the serial console. The following
jumper settings are default on these boards, and are required to connect the
UART signals to the USB bridge circuit:

- MIMXRT1170-EVKB: JP2 open (default)
- MIMXRT1170-EVK:  J31 and J32 shorted (default)

Connect a USB cable from your PC to the on-board debugger USB port:

- MIMXRT1170-EVKB: J86
- MIMXRT1170-EVK:  J11

Use the following settings with your serial terminal of choice (minicom, putty,
etc.):

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flashing
========

Here is an example for the :zephyr:code-sample:`hello_world` application.

Before powering the board, make sure SW1 is set to 0001b

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mimxrt1170_evk/mimxrt1176/cm7
   :goals: flash

Power off the board, and change SW1 to 0010b. Then power on the board and
open a serial terminal, reset the board (press the SW4 button), and you should
see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v3.4.0-xxxx-xxxxxxxxxxxxx *****
   Hello World! mimxrt1170_evk

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mimxrt1170_evk/mimxrt1176/cm7
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v3.4.0-xxxx-xxxxxxxxxxxxx *****
   Hello World! mimxrt1170_evk

ENET1G Driver
=============

Current default of ethernet driver is to use 100M Ethernet instance ENET.
To use the 1G Ethernet instance ENET1G, include the overlay to west build with
the option ``-DEXTRA_DTC_OVERLAY_FILE=nxp,enet1g.overlay`` instead.

.. include:: ../../common/board-footer.rst
   :start-after: nxp-board-footer

.. _MIMXRT1170-EVK Website:
   https://www.nxp.com/design/development-boards/i-mx-evaluation-and-development-boards/i-mx-rt1170-evaluation-kit:MIMXRT1170-EVK

.. _MIMXRT1170-EVK Board Hardware User's Guide:
   https://www.nxp.com/webapp/Download?colCode=MIMXRT1170EVKHUG

.. _i.MX RT1170 Website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/i-mx-rt-crossover-mcus/i-mx-rt1170-crossover-mcu-family-first-ghz-mcu-with-arm-cortex-m7-and-cortex-m4-cores:i.MX-RT1170

.. _i.MX RT1170 Datasheet:
   https://www.nxp.com/docs/en/data-sheet/IMXRT1170CEC.pdf

.. _i.MX RT1170 Reference Manual:
   https://www.nxp.com/webapp/Download?colCode=IMXRT1170RM

.. _Using J-Link with MIMXRT1160-EVK or MIMXRT1170-EVK:
   https://community.nxp.com/t5/i-MX-RT-Knowledge-Base/Using-J-Link-with-MIMXRT1160-EVK-or-MIMXRT1170-EVK/ta-p/1529760

.. _Using J-Link with MIMXRT1170-EVKB:
   https://community.nxp.com/t5/i-MX-RT-Knowledge-Base/Using-J-Link-with-MIMXRT1170-EVKB/ta-p/1715138

.. _AN13264:
   https://www.nxp.com/docs/en/application-note/AN13264.pdf

.. _NXP MCUXpresso for Visual Studio Code:
	https://www.nxp.com/design/software/development-software/mcuxpresso-software-and-tools-/mcuxpresso-for-visual-studio-code:MCUXPRESSO-VSC
