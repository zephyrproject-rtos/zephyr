.. zephyr:board:: ra8p1_titan

Overview
********

The RA8P1 Titan Board is a development board launched by RT-Thread, based on the
Renesas R7KA8P1 chip featuring a dual-core architecture with Cortex-M85 and
Cortex-M33. It provides engineers with a flexible and comprehensive development
platform, helping developers gain deeper insights and experiences in the field
of embedded IoT.

The RA8P1 MCU incorporates a high-performance Arm® Cortex®-M85 core running up to
1 GHz and Arm® Cortex®-M33 core running up to 250 MHz with the following features:

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

Key Features
============

- Renesas RA8P1 MCU with 1 GHz Arm Cortex-M85, 250 MHz Arm Cortex-M33,
  Arm Ethos-U55 NPU, and 1 MB MRAM
- 64 MB NorFlash and 32 MB HyperRAM memory extension
- 2x Gbps Ethernet
- Octal SPI, Wi-Fi, CAN FD, USBFS/HS, and SDHI
- On-board IMU and magnetometer

More information about the board can be found at the `RA8P1 Titan Board GitHub`_
repository and the `RT-Thread RA8P1 Titan Board introduction`_.

Hardware
********

Detailed hardware information can be found at:

- RA8P1 MCU: `RA8P1 Group User's Manual Hardware`_
- RA8P1 Titan Board: `RA8P1 Titan Board GitHub`_

Board Hardware
==============

The RA8P1 Titan Board is designed around the Renesas R7KA8P1 MCU and exposes
the RA8P1 device features through memory, networking, storage, wireless,
sensor, and expansion interfaces.

MCU and Memory
--------------

- Renesas R7KA8P1 MCU
- Arm Cortex-M85 core running up to 1 GHz
- Arm Cortex-M33 core running up to 250 MHz
- Arm Ethos-U55 NPU for AI acceleration
- 1 MB on-chip MRAM
- 2 MB on-chip SRAM, including CM85 TCM, CM33 TCM, and user SRAM
- 64 MB external NorFlash
- 32 MB external HyperRAM

Connectivity and Storage
------------------------

- Two gigabit Ethernet interfaces
- Wi-Fi module
- CAN FD interface
- USB Full-Speed and USB High-Speed interfaces
- SDHI interface
- Octal SPI interface

Sensors and Expansion
---------------------

- On-board IMU
- On-board magnetometer
- Board interfaces for networking, storage, wireless, and expansion

Supported Features
==================

.. zephyr:board-supported-hw::

Dual Core Operation
*******************

The RA8P1 Titan Board supports dual core operation with both the Cortex-M85
(CPU0) and Cortex-M33 (CPU1) cores. By default, the CM85 core is the boot core
and is responsible for initializing the system and starting the CM33 core.

Memory Usage
============

By default, MRAM (Flash) and SRAM are split evenly between the two cores.
Users can manually change the address and size for MRAM (Flash) and SRAM with
the following nodes:

   - CPU0: &code_mram_cm85, &sram0
   - CPU1: &code_mram_cm33, &sram1

.. note::

   - MRAM usable range: 0x0200_0000 ... 0x0210_0000 (1 MB)
   - SRAM usable range: 0x2200_0000 ... 0x221A_0000 (1664 KB)

Dual Core Flashing
==================

When flashing or debugging dual-core samples,
:kconfig:option:`CONFIG_SOC_RA_ENABLE_START_SECOND_CORE` must be selected
for the CM85 image.
The CM85 core is responsible for starting the CM33 core in
``soc_late_init_hook``.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``ra8p1_titan`` board configuration can be built, flashed,
and debugged in the usual way. See :ref:`build_an_application` and
:ref:`application_run` for more details on building and running.

Serial Console
==============

The CM85 board target uses UART5 for the Zephyr console and shell. The CM33
board target uses UART6. These console UARTs are available on the external UART
interface pins and are not connected to the on-board DAP-Link virtual COM port.

The DAP-Link virtual COM port is wired to a different UART on this board. That
UART is also used by the Wi-Fi/Bluetooth module UART HCI interface, so it is not
the default Zephyr log console for either core. Use an external USB-to-UART
adapter connected to the UART5 or UART6 pins when reading Zephyr logs.

Here is an example for the :zephyr:code-sample:`hello_world` application on the
CM85 core.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ra8p1_titan/r7ka8p1kflcac/cm85
   :goals: flash

Open a serial terminal, reset the board, and you should see the following
message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v4.2.0-xxx-xxxxxxxxxxxxx *****
   Hello World! ra8p1_titan/r7ka8p1kflcac/cm85

For the CM33 core, use the ``--sysbuild`` flow to build a minimal first-core
launcher image that starts the CM33 core.

.. zephyr-app-commands::
   :tool: west
   :zephyr-app: samples/hello_world
   :board: ra8p1_titan/r7ka8p1kflcac/cm33
   :goals: build flash
   :west-args: --sysbuild

Flashing
========

Programs can be flashed to the RA8P1 Titan Board using the configured Zephyr
runner.

To flash a program to the board:

1. Connect the board debug port to the host PC.

2. Execute the west command:

   .. code-block:: console

      west flash

MCUboot Bootloader
==================

Sysbuild can build and flash all images needed to bootstrap the board.

To build the sample application using sysbuild:

.. zephyr-app-commands::
   :tool: west
   :zephyr-app: samples/hello_world
   :board: ra8p1_titan/r7ka8p1kflcac/cm85
   :goals: build flash
   :west-args: --sysbuild
   :gen-args: -DSB_CONFIG_BOOTLOADER_MCUBOOT=y

By default, sysbuild creates MCUboot and user application images.

The build directory structure created by sysbuild is different from a
traditional Zephyr build. Output is structured by domain subdirectories:

.. code-block::

   build/
   |-- hello_world
   |   `-- zephyr
   |       |-- zephyr.elf
   |       |-- zephyr.hex
   |       |-- zephyr.bin
   |       |-- zephyr.signed.bin
   |       `-- zephyr.signed.hex
   |-- mcuboot
   |   `-- zephyr
   |       |-- zephyr.elf
   |       |-- zephyr.hex
   |       `-- zephyr.bin
   `-- domains.yaml

.. note::

   With the ``--sysbuild`` option, MCUboot is rebuilt and reflashed every time a
   pristine build is used.

To flash only the user application in subsequent builds, use:

.. code-block:: console

   west flash --domain hello_world

For more information about system build, see the :ref:`sysbuild` documentation.

You should see the following message in the terminal:

.. code-block:: console

   *** Booting MCUboot v2.2.0-171-g8513be710e5e ***
   *** Using Zephyr OS build v4.2.0-6156-ged85ac9ffda9 ***
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
   *** Booting Zephyr OS build v4.2.0-6156-ged85ac9ffda9 ***
   Hello World! ra8p1_titan/r7ka8p1kflcac/cm85

References
**********

- `RA8P1 Titan Board GitHub`_
- `RT-Thread RA8P1 Titan Board introduction`_
- `RA8P1 MCU group Website`_

.. target-notes::

.. _RA8P1 Titan Board GitHub:
   https://github.com/RT-Thread-Studio/sdk-bsp-ra8p1-titan-board

.. _RT-Thread RA8P1 Titan Board introduction:
   https://www.renesas.com/en/document/prb/rt-thread-ra8p1-titan-board

.. _RA8P1 MCU group Website:
   https://www.renesas.com/en/products/microcontrollers-microprocessors/ra-cortex-m-mcus/ra8p1-1ghz-arm-cortex-m85-and-ethos-u55-npu-based-ai-microcontroller

.. _RA8P1 Group User's Manual Hardware:
   https://www.renesas.com/us/en/document/mah/ra8p1-group-users-manual-hardware
