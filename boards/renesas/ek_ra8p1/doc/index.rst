.. zephyr:board:: ek_ra8p1

Overview
********

The EK-RA8P1 is an Evaluation Kit for Renesas RA8P1 MCU Group which integrates multiple series of software-compatible
Arm®-based 32-bit cores that share a common set of Renesas peripherals to facilitate design scalability and efficient
platform-based product development.

The MCU in this series incorporates a high-performance Arm® Cortex®-M85 core running up to 1 GHz and Arm®
Cortex®-M33 core running up to 250 MHz with the following features:

- Up to 1 MB MRAM
- 2 MB SRAM (256 KB of CM85 TCM RAM, 128 KB CM33 TCM RAM, 1664 KB of user SRAM)
- Arm® Ethos™-U55 NPU
- Octal Serial Peripheral Interface (OSPI)
- Layer 3 Ethernet Switch Module (ESWM), USBFS, USBHS, SD/MMC Host Interface
- Graphics LCD Controller (GLCDC)
- 2D Drawing Engine (DRW)
- MIPI DSI/CSI interface
- Analog peripherals
- Security and safety features

**MCU Native Pin Access**

- 1 GHz Arm Cortex-M85 and 250 MHz Arm Cortex-M33 based RA8P1 MCU in 289 pins, BGA package
- Native pin access through 2 x 20-pin, and 2 x 40-pin headers (no populated)
- Camera Expansion connector (present on the underside of the EK-RA8P1 board)
- 2-Lane MIPI Display connector (present on the underside of the EK-RA8P1 board)
- Parallel graphics display interface connector
- MCU current measurement points for precision current consumption measurement
- Multiple clock sources - RA8P1 MCU oscillator and sub-clock oscillator crystals,
  providing precision 24.000 MHz and 32,768 Hz reference clocks.
  Additional low precision clocks are available internal to the RA8P1 MCU

**System Control and Ecosystem Access**

- USB Full Speed Host and Device (USB-C connector)
- Four 5V input sources

  - USB (Debug, Full Speed, High Speed)
  - External power supply (using surface mount clamp test points and power input vias)

- Three Debug modes

  - Debug on-board (SWD and JTAG)
  - Debug in (ETM, SWD, SWO and JTAG)
  - Debug out (SWD, SWO, and JTAG)

- User LEDs and buttons

  - Three User LEDs (red, blue, green)
  - Power LED (white) indicating availability of regulated power
  - Debug LED (yellow) indicating the debug connection
  - Ethernet LEDs (amber, yellow, green)
  - Two User buttons
  - One Reset button

- Five most popular ecosystems expansions

  - Two Seeed Grove system (I2C/I3C/Analog) connectors (not populated)
  - One SparkFun Qwiic connector (not populated)
  - Two Digilent Pmod (SPI, UART and I2C) connectors
  - Arduino (Uno R3) connector
  - MikroElektronika mikroBUS connector (not populated)

- MCU boot configuration jumper

**Special Feature Access**

- Ethernet (RJ45 RGMII interface)
- USB High Speed Host and Device (USB-C connector)
- 512 Mb (64 MB) External Octo-SPI Flash (present in the MCU Native Pin Access area of the EK-RA8P1 board)

Hardware
********

Detailed hardware features can be found at:

- RA8P1 MCU: `RA8P1 Group User's Manual Hardware`_
- EK-RA8P1 board: `EK-RA8P1 - User's Manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

.. note::

   - Other hardware features are currently not supported by the port.

Dual Core Operation
*******************

The EK-RA8P1 supports dual core operation with both the Cortex-M85 (CPU0) and Cortex-M33 (CPU1) cores.
By default, the CM85 core is the boot core and is responsible for initializing the system and
starting the CM33 core.

Memory Usage
============

By default, Flash (MRAM) and SRAM are split evenly between the two cores.
Users can manually change the address and size for Flash and SRAM as follows node:

   - CPU0: &flash0, &sram0
   - CPU1: &flash1, &sram1

.. note::

   - Flash usable range: 0x0200_0000 ... 0x0290_0000
   - SRAM usable range: 0x2200_0000 ... 0x221D_4000

Dual Core Flashing
==================

When flashing or debugging dual-core samples, ensure that CONFIG_SOC_RA_ENABLE_START_SECOND_CORE is selected
for the CM85 image. The CM85 core is responsible for starting the CM33 core in soc_late_init_hook.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``ek_ra8p1`` board configuration can be
built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

Here is an example for the :zephyr:code-sample:`hello_world` application on CM85 core.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ek_ra8p1/r7ka8p1kflcac/cm85
   :goals: flash

Open a serial terminal, reset the board (press the S3 button), and you should
see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v4.2.0-xxx-xxxxxxxxxxxxx *****
   Hello World! ek_ra8p1/r7ka8p1kflcac/cm85

Flashing
========

Program can be flashed to EK-RA8P1 via the on-board SEGGER J-Link debugger.
SEGGER J-link's drivers are available at https://www.segger.com/downloads/jlink/

To flash the program to board

1. Connect to J-Link OB via USB port to host PC

2. Make sure J-Link OB jumper is in default configuration as described in `EK-RA8P1 - User's Manual`_

3. Execute west command

	.. code-block:: console

		west flash -r jlink

References
**********
- `EK-RA8P1 Website`_
- `RA8P1 MCU group Website`_

.. _EK-RA8P1 Website:
   https://www.renesas.com/en/design-resources/boards-kits/ek-ra8p1

.. _RA8P1 MCU group Website:
   https://www.renesas.com/en/products/microcontrollers-microprocessors/ra-cortex-m-mcus/ra8p1-1ghz-arm-cortex-m85-and-ethos-u55-npu-based-ai-microcontroller

.. _EK-RA8P1 - User's Manual:
   https://www.renesas.com/en/document/mat/ek-ra8p1-v1-users-manual

.. _RA8P1 Group User's Manual Hardware:
   https://www.renesas.com/en/document/mah/ra8p1-group-users-manual-hardware
