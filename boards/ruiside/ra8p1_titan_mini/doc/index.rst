.. zephyr:board:: ra8p1_titan_mini

Overview
********

The Titan Board Mini is a development board launched by RT-Thread, based on the
Renesas R7KA8P1 chip featuring a dual-core architecture with Cortex-M85 and
Cortex-M33. It provides engineers with a flexible and comprehensive development
platform, helping developers gain deeper insights and experiences in the field
of embedded IoT.

The RA8P1 MCU incorporates a high-performance Arm® Cortex®-M85 core running up to
1 GHz and an Arm® Cortex®-M33 core running up to 250 MHz with the following
features:

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

- Renesas R7KA8P1 MCU in 289-pin FBGA package
- Arm Cortex-M85 core running up to 1 GHz
- Arm Cortex-M33 core running up to 250 MHz
- Arm Ethos-U55 NPU for AI acceleration
- Up to 1 MB MRAM and 2 MB SRAM
- Two external ports with gigabit Ethernet support
- TSN support, including IEEE 802.1AS-2020, IEEE 802.1Qbv/802.3br, and
  IEEE 802.1Qbu
- Network redundancy support, including DLR and PRP
- Host interfaces: parallel, serial, dual, quad, and octal
- Memory interfaces: Octa-SPI, Quad-SPI, HyperRAM, HyperFLASH, and SRAM
- UART, RS-485, CAN FD, I2C, SPI, OSPI, SDHI, USBHS, and USBFS interfaces
- PWM timers, ADC, RTC, watchdog, PDM microphone, IMU, display, and camera
  examples in the official RT-Thread SDK
- SWD debug interface with J-Link debugger support

More information about the board can be found in the
`RA8P1 Titan Mini Board GitHub`_ repository.

Hardware
********

Detailed hardware information can be found at:

- RA8P1 MCU: `RA8P1 Group User's Manual Hardware`_
- Titan Board Mini: `RA8P1 Titan Mini Board GitHub`_

Supported Features
==================

.. zephyr:board-supported-hw::

Dual Core Operation
*******************

The Titan Board Mini supports dual core operation with both the Cortex-M85
(CPU0) and Cortex-M33 (CPU1) cores. By default, the CM85 core is the boot core
and is responsible for initializing the system and starting the CM33 core.

Memory Usage
============

By default, MRAM (Flash) and SRAM are split evenly between the two cores.
Users can manually change the address and size for MRAM (Flash) and SRAM with
the following nodes:

- CPU0: ``&code_mram_cm85``, ``&sram0``
- CPU1: ``&code_mram_cm33``, ``&sram1``

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

Applications for the ``ra8p1_titan_mini`` board configuration can be built,
flashed, and debugged in the usual way. See :ref:`build_an_application` and
:ref:`application_run` for more details on building and running.

Serial Console
==============

Both the CM85 and CM33 board targets use UART2 for the Zephyr console and shell.
UART2 is available on the external UART interface pins. Use an external USB-to-UART
adapter connected to the UART2 pins when reading Zephyr logs.

Here is an example for the :zephyr:code-sample:`hello_world` application on the
CM85 core.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ra8p1_titan_mini/r7ka8p1kflcac/cm85
   :goals: flash

Open a serial terminal, reset the board, and you should see the following
message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v4.2.0-xxx-xxxxxxxxxxxxx *****
   Hello World! ra8p1_titan_mini/r7ka8p1kflcac/cm85

For the CM33 core, use the ``--sysbuild`` flow to build a minimal first-core
launcher image that starts the CM33 core.

.. zephyr-app-commands::
   :tool: west
   :zephyr-app: samples/hello_world
   :board: ra8p1_titan_mini/r7ka8p1kflcac/cm33
   :goals: build flash
   :west-args: --sysbuild

Flashing
========

Programs can be flashed to the Titan Board Mini using a J-Link debugger over the
SWD interface.

To flash a program to the board:

1. Connect the J-Link debugger to the board SWD interface and the host PC.

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
   :board: ra8p1_titan_mini/r7ka8p1kflcac/cm85
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
   Hello World! ra8p1_titan_mini/r7ka8p1kflcac/cm85

References
**********

- `RA8P1 Titan Mini Board GitHub`_
- `RA8P1 MCU group Website`_

.. target-notes::

.. _RA8P1 Titan Mini Board GitHub:
   https://github.com/RT-Thread-Studio/sdk-bsp-ra8p1-titan-mini

.. _RA8P1 MCU group Website:
   https://www.renesas.com/en/products/microcontrollers-microprocessors/ra-cortex-m-mcus/ra8p1-1ghz-arm-cortex-m85-and-ethos-u55-npu-based-ai-microcontroller

.. _RA8P1 Group User's Manual Hardware:
   https://www.renesas.com/us/en/document/mah/ra8p1-group-users-manual-hardware
