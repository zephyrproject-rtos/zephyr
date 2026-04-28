.. zephyr:board:: ek_ra8d2

Overview
********

The EK-RA8D2 is an Evaluation Kit for Renesas RA8D2 MCU Group which integrates multiple series of software-compatible
Arm®-based 32-bit cores that share a common set of Renesas peripherals to facilitate design scalability and efficient
platform-based product development

The MCU in this series incorporates a high-performance Arm® Cortex®-M85 core running up to 1 GHz and Arm® Cortex®-M33
core running up to 250 MHz with the following features:

- Up to 1 MB MRAM
- 2 MB SRAM (256 KB of CM85 TCM RAM, 128 KB CM33 TCM RAM, 1664 KB of user SRAM)
- Octal Serial Peripheral Interface (OSPI)
- Layer 3 Ethernet Switch Module (ESWM), USBFS, USBHS, SD/MMC Host Interface
- Graphics LCD Controller (GLCDC)
- 2D Drawing Engine (DRW)
- MIPI DSI/CSI interface
- Analog peripherals
- Security and safety features

**MCU Native Pin Access**

- 1 GHz Arm® Cortex®-M85 core and 250 MHz Arm® Cortex®-M33 core based RA8D2 MCU in 289 pins, BGA package
- 1 MB MRAM, 2 MB SRAM with ECC
- Native pin access through 2 x 20-pin, and 2 x 40-pin headers (not populated)
- Parallel Graphics Expansion Port
- Camera Expansion Port (present at the underside of the EK-RA8D2 board)
- MIPI Graphics Expansion Port (present at the underside of the EK-RA8D2 board)
- MCU current measurement points for precision current consumption measurement
- Multiple clock sources - RA MCU oscillator and sub-clock oscillator crystals,
  providing precision 24.000 MHz and 32,768 Hz reference clocks.
  Additional low-precision clocks are available internal to the RA8D2 MCU

**System Control and Ecosystem Access**

- Four 5 V input sources

  - USB (Debug, Full Speed, High Speed)
  - External power supply (using surface mount clamp test points and power input vias)

- Three Debug modes

  - Debug on-board (SWD and JTAG)
  - Debug in (ETM, SWD, SWO, and JTAG)
  - Debug out (SWD, SWO, and JTAG)

- User LEDs, Status LEDs and switches

  - Three User LEDs (red, blue, green)
  - Power LED (white) indicating availability of regulated power.
  - Debug LED (yellow) indicating the debug connection.
  - Ethernet LEDs (amber, yellow, green)
  - Two User switches, One Reset switch

- Five most popular ecosystems expansions

  - Two Seeed Grove® system (I2C/I3C/Analog) connectors (not populated)
  - SparkFun® Qwiic® connector (not populated)
  - Two Digilent PmodTM (SPI, UART and I2C) connectors
  - Arduino™ (Uno R3) connector
  - MikroElektronikaTM mikroBUS™ connector (not populated)

- USB Full Speed Host and Device (USB-C connector)
- MCU boot configuration jumper

**Special Feature Access**

- USB High Speed Host and Device (USB-C connector)
- Ethernet (RJ45 RGMII interface)
- 64 MB (512 Mb) External Octo-SPI Flash (present in the MCU Native Pin Access area)
- 64 MB (512 Mb) SDRAM (present in the MCU Native Pin Access area)
- PDM MEMS Microphones (present at the underside of the EK-RA8D2 board)
- Audio CODEC with speaker out connections
- Configuration switches

Hardware
********

Detailed hardware features can be found at:

- RA8D2 MCU: `RA8D2 Group User's Manual Hardware`_
- EK-RA8D2 board: `EK-RA8D2 - User's Manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

.. note::

   - For using the Camera Expansion Port (J35) in DVP interface, please set switch SW4 as following configuration:

     +-------------+-------------+----------------+---------------+-----------+------------+-------------+-------------+
     | SW4-1 PMOD1 | SW4-2 PMOD1 | SW4-3 Octo-SPI | SW4-4 Arduino | SW4-5 I3C | SW4-6 MIPI | SW4-7 USBFS | SW4-8 USBHS |
     +-------------+-------------+----------------+---------------+-----------+------------+-------------+-------------+
     |      -      |      -      |        -       |       -       |    OFF    |     ON     |      -      |      -      |
     +-------------+-------------+----------------+---------------+-----------+------------+-------------+-------------+


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``ek_ra8d2`` board configuration can be
built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

Here is an example for the :zephyr:code-sample:`hello_world` application on CM85 core.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ek_ra8d2/r7ka8d2kflcac/cm85
   :goals: flash

Open a serial terminal, reset the board (Pressing the reset switch SW3), and you should
see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v4.2.0-xxx-xxxxxxxxxxxxx *****
   Hello World! ek_ra8d2/r7ka8d2kflcac/cm85

Flashing
========

Program can be flashed to EK-RA8D2 via the on-board SEGGER J-Link debugger.
SEGGER J-link's drivers are available at https://www.segger.com/downloads/jlink/

To flash the program to board

1. Connect to J-Link OB via USB port to host PC

2. Make sure J-Link OB jumper is in default configuration as described in `EK-RA8D2 - User's Manual`_

3. Execute west command

	.. code-block:: console

		west flash -r jlink

MCUboot bootloader
==================

The sysbuild makes possible to build and flash all necessary images needed to
bootstrap the board.

To build the sample application using sysbuild use the command:

.. zephyr-app-commands::
   :tool: west
   :zephyr-app: samples/hello_world
   :board: ek_ra8d2/r7ka8d2kflcac/cm85
   :goals: build flash
   :west-args: --sysbuild
   :gen-args: -DSB_CONFIG_BOOTLOADER_MCUBOOT=y

By default, Sysbuild creates MCUboot and user application images.

Build directory structure created by sysbuild is different from traditional
Zephyr build. Output is structured by the domain subdirectories:

.. code-block::

  build/
  ├── hello_world
  |    └── zephyr
  │       ├── zephyr.elf
  │       ├── zephyr.hex
  │       ├── zephyr.bin
  │       ├── zephyr.signed.bin
  │       └── zephyr.signed.hex
  ├── mcuboot
  │    └── zephyr
  │       ├── zephyr.elf
  │       ├── zephyr.hex
  │       └── zephyr.bin
  └── domains.yaml

.. note::

   With ``--sysbuild`` option, MCUboot will be rebuilt and reflashed
   every time the pristine build is used.

To only flash the user application in the subsequent builds, Use:

.. code-block:: console

   $ west flash --domain hello_world

For more information about the system build please read the :ref:`sysbuild` documentation.

You should see the following message in the terminal:

.. code-block:: console

   *** Booting MCUboot v2.2.0-171-g8513be710e5e ***
   *** Using Zephyr OS build v4.2.0-6343-g2ce9ea10e7df ***
   I: Starting bootloader
   I: Image index: 0, Swap type: none
   I: Image index: 0, Swap type: none
   I: Primary image: magic=unset, swap_type=0x1, copy_done=0x3, image_ok=0x3
   I: Secondary image: magic=unset, swap_type=0x1, copy_done=0x3, image_ok=0x3
   I: Boot source: none
   I: Image index: 0, Swap type: none
   I: Image index: 0, Swap type: none
   I: Image index: 0, Swap type: none
   I: Image index: 0, Swap type: none
   I: Bootloader chainload address offset: 0x10000
   I: Image version: v0.0.0
   I: Jumping to the first image slot
   *** Booting Zephyr OS build v4.2.0-6343-g2ce9ea10e7df ***
   Hello World! ek_ra8d2/r7ka8d2kflcac/cm85

References
**********
- `EK-RA8D2 Website`_
- `RA8D2 MCU group Website`_

.. _EK-RA8D2 Website:
   https://www.renesas.com/en/design-resources/boards-kits/ek-ra8d2

.. _RA8D2 MCU group Website:
   https://www.renesas.com/en/products/ra8d2

.. _EK-RA8D2 - User's Manual:
   https://www.renesas.com/en/document/mat/ek-ra8d2-v1-users-manual

.. _RA8D2 Group User's Manual Hardware:
   https://www.renesas.com/en/document/mah/ra8d2-group-users-manual-hardware
