.. zephyr:board:: stm32mp257f_ev1/stm32mp257fxx/m33

Overview
********

The STM32MP257F-EV1 Evaluation board is designed as a complete demonstration
and development platform for the STMicroelectronics STM32MP257F microprocessor
based on Arm |reg| dual-core Cortex |reg|-A35 (1.5 GHz) and Cortex |reg|-M33
(400 MHz), and the STPMIC25APQR companion chip.
Zephyr OS is ported to run on the Cortex |reg|-M33 core, as a coprocessor of
the Cortex |reg|-A35 core.

Features:
=========

- STM32MP257FAI3 microprocessor based a Arm |reg| dual-core Cortex |reg|-A35, a
  Cortex |reg|-M33 and a Cortex |reg|-M0+ in a TFBGA436 package
- ST power management STPMIC25APQR
- Two 16-Gbit DDR4 DRAMs
- 512-Mbit (64 Mbytes) S-NOR flash memory
- 32-Gbit (4 Gbytes) eMMC v5.0
- Three 1-Gbit/s Ethernet (RGMII) with TSN switch compliant with IEEE-802.3ab
- High-speed USB Host 2-port hub
- High-speed USB Type-C |reg| DRP
- Four user LEDs
- Two user, one tamper, and one reset push-buttons
- One wake-up button
- Four boot pin switches
- Board connectors:

  - Three Ethernet RJ45
  - Two USB Host Type-A
  - USB Type-C |reg|
  - microSD |trade| card holder
  - Mini PCIe
  - Dual-lane MIPI CSI-2 |reg| camera module expansion connector
  - Two CAN FD
  - LVDS
  - MIPI10
  - GPIO expansion connector
  - mikroBUS |trade| expansion connector
  - VBAT for power backup

- On-board STLINK-V3EC debugger/programmer with USB re-enumeration capability
  Two Virtual COM ports (VCPs), and debug ports (JTAG/SWD)
- Mainlined open-source Linux |reg| STM32 MPU OpenSTLinux Distribution and
  STM32CubeMP2 software with examples
- Linux |reg| Yocto project, Buildroot, and STM32CubeIDE as
  development environments

More information about the board can be found at the
`STM32MP257F-EV1 website`_.

Hardware
********

Cores:
======

- Up to 64-bit dual-core Arm |reg| Cortex |reg|-A35
  - Up to 1.5 GHz
  - 32-Kbyte I + 32-Kbyte D level 1 cache for each core
  - 512-Kbyte unified level 2 cache
  - Arm |reg| NEON |trade| and Arm |reg| TrustZone |reg|
- 32-bit Arm |reg| Cortex |reg|-M33 with FPU/MPU
  - Up to 400 MHz
  - L1 16-Kbyte I / 16-Kbyte D
  - Arm |reg| TrustZone |reg|
- 32-bit Arm |reg| Cortex |reg|-M0+ in SmartRun domain
  - Up to 200 MHz (up to 16 MHz in autonomous mode)

Memories:
=========

- External DDR memory up to 4 Gbytes
  - Up to DDR3L-2133 16/32-bit
  - Up to DDR4-2400 16/32-bit
  - Up to LPDDR4-2400 16/32-bit
- 808-Kbyte internal SRAM: 256-Kbyte AXI SYSRAM, 128-Kbyte AXI video RAM or
  SYSRAM extension, 256-Kbyte AHB SRAM, 128-Kbyte AHB SRAM with ECC in backup
  domain, 8-Kbyte SRAM with ECC in backup domain, 32 Kbytes in SmartRun domain
- Two Octo-SPI memory interfaces
- Flexible external memory controller with up to 16-bit data bus: parallel
  interface to connect external ICs, and SLC NAND memories with up to 8-bit ECC

Power
=====

- STPMIC25 for voltage regulation (multiple buck/LDO regulators)
- USB-C or 5V DC jack power input
- VBAT backup battery connector (RTC, backup SRAM)

Clock management
================

- External oscillators:
  - 32.768 kHz LSE crystal
  - 40 MHz HSE crystal
- Internal oscillators:
  - 64 MHz HSI oscillator
  - 4 MHz CSI oscillator
  - 32 kHz LSI oscillator
  - Five separate PLLs with integer and fractional mode

Security/Safety
===============

- Secure boot, TrustZone |reg| peripherals, active tamper, environmental
  monitors, display secure layers, hardware accelerators
- Complete resource isolation framework

Connectivity
============

- 3x Gigabit Ethernet (RGMII, TSN switch capable)
- 2x CAN FD
- USB 2.0 High-Speed Host (dual-port)
- USB Type-C |reg| DRP
- mikroBUS |trade| expansion
- GPIO expansion connector

Display & Camera
================

- DSI interface (4-lane)
- LVDS interface (4-lane)
- Camera CSI-2 interface (2-lane)

Debug
=====

- STLINK-V3EC (onboard debugger with VCP, JTAG and SWD)

More information about STM32MP257F can be found here:

- `STM32MP257F on www.st.com`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

STM32MP257F-EV1 Evaluation Board schematic is available here:
`STM32MP257F-EV1 Evaluation board schematics`_

System Clock
============

Cortex |reg|-A35
----------------

Not yet supported in Zephyr.

Cortex |reg|-M33
----------------

The Cortex |reg|-M33 Core is configured to run at a 400 MHz clock speed.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Prerequisite
============

Before you can run Zephyr on the STM32MP257F-EV1 Evaluation board, you need to
set up the Cortex |reg|-A35 core with a Linux |reg| environment. The Cortex
|reg|-M33 core runs Zephyr as a coprocessor, and it requires the Cortex
|reg|-A35 to load and start the firmware using remoteproc.

One way to set up the Linux environment is to use the official ST
OpenSTLinux distribution, following the `Starter Package`_. (more information
about the procedure can be found in the `STM32MPU Wiki`_)

Once Linux is running on the Cortex-A35, you can proceed with building and
loading Zephyr firmware for the Cortex-M33 core as described below.

The STM32MP257F-EV1 Evaluation board has two different ways to load the
firmware on the Cortex |reg|-M33 core, either from the SRAM or from the DDR.

The Cortex |reg| -A35 is responsible for loading the binary to the correct
memory location and starting the firmware. This is done using the 'remoteproc'
framework. Remoteproc checks the different memory sections in the ELF file
looking for the vector table.

By default, it expects the section name to be '.isr_vectors'. In Zephyr, the
vector table is located in the 'rom_start' section. The solution implemented is
to rename the 'rom_start' section to '.isr_vectors' in the Zephyr build system.
The section name is changed in the linker script located in
:zephyr_file:`soc/st/stm32/stm32mp2x/m33/linker.ld`

Loading the firmware
====================
In the case of the MP boards, the Cortex |reg| -A35 is responsible (in the
current distribution) of loading the firmware image and starting the
Cortex |reg| -M33 core. The application can be built using west, taking the
:zephyr:code-sample:`blinky` as an example.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: stm32mp257f_ev1/stm32mp257fxx/m33
   :goals: build

The firmware can be copied to the board file system and started with the Linux
remoteproc framework. (more information about the procedure can be found in the
`STM32MP257F boot Cortex-M33 firmware`_)

Debugging
=========
Applications can be debugged using OpenOCD and GDB. The firmware must first be
started by the Cortex |reg|-A35. The debugger can then be attached to the
running Zephyr firmware using OpenOCD.

- Build the sample:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: stm32mp257f_ev1/stm32mp257fxx/m33
   :goals: build

- Copy the firmware to the board, load it and start it with remoteproc
  (`STM32MP257F boot Cortex-M33 firmware`_). The orange LED should be blinking.
- Attach to the target:

.. code-block:: console

   $ west attach

References
==========

.. target-notes::

.. _STM32MP257F-EV1 website:
  https://www.st.com/en/evaluation-tools/stm32mp257f-ev1.html#overview

.. _STM32MP257F-EV1 Evaluation board User Manual:
  https://www.st.com/resource/en/user_manual/um3359-evaluation-board-with-stm32mp257f-mpu-stmicroelectronics.pdf

.. _STM32MP257F-EV1 Evaluation board schematics:
  https://www.st.com/resource/en/schematic_pack/mb1936-mp257f-x-d01-schematic.pdf

.. _STM32MP25xC/F Evaluation board datasheet:
  https://www.st.com/resource/en/datasheet/stm32mp257c.pdf

.. _STM32MP257F on www.st.com:
  https://www.st.com/en/microcontrollers-microprocessors/stm32mp257f.html

.. _STM32MP257F reference manual:
  https://www.st.com/resource/en/reference_manual/rm0457-stm32mp25xx-advanced-armbased-3264bit-mpus-stmicroelectronics.pdf

.. _STM32MP257F boot Cortex-M33 firmware:
  https://wiki.st.com/stm32mpu/wiki/Linux_remoteproc_framework_overview#Remote_processor_boot_through_sysfs

.. _Starter Package:
  https://wiki.st.com/stm32mpu/wiki/STM32MP25_Evaluation_boards_-_Starter_Package

.. _STM32MPU Wiki:
  https://wiki.st.com/stm32mpu/wiki/Main_Page
