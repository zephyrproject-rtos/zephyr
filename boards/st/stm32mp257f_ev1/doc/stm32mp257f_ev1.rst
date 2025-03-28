.. zephyr:board:: stm32mp257f_ev1

Overview
********

The STM32MP257F-EV1 Evaluation board is designed as a complete demonstration
and development platform for the STMicroelectronics STM32MP257F microprocessor
based on Arm® dual-core Cortex®-A35 (1.5 GHz) and Cortex®-M33 (400 MHz), and
the STPMIC25APQR companion chip.
Zephyr OS is ported to run on the Cortex®-M33 core, as a coprocessor of the
Cortex®-A35 core.

Features:
=========

- STM32MP257FAI3 microprocessor based on the Arm® dual-core Cortex®-A35 (1.5
  GHz) and Cortex®-M33 (400 MHz) in a TFBGA436 package
- A Cortex-M0+
- ST power management STPMIC25APQR
- Two 16-Gbit DDR4 DRAMs
- 512-Mbit (64 Mbytes) S-NOR flash memory
- 32-Gbit (4 Gbytes) eMMC v5.0
- Three 1-Gbit/s Ethernet (RGMII) with TSN switch compliant with IEEE-802.3ab
- High-speed USB Host 2-port hub
- High-speed USB Type-C® DRP
- Four user LEDs
- Two user, one tamper, and one reset push-buttons
- One wake-up button
- Four boot pin switches
- Board connectors:

  - Three Ethernet RJ45
  - Two USB Host Type-A
  - USB Type-C®
  - microSD™ card holder
  - Mini PCIe
  - Dual-lane MIPI CSI-2® camera module expansion connector
  - Two CAN FD
  - LVDS
  - MIPI10
  - GPIO expansion connector
  - mikroBUS™ expansion connector
  - VBAT for power backup

- On-board STLINK-V3EC debugger/programmer with USB re-enumeration capability
  Two Virtual COM ports (VCPs), and debug ports (JTAG/SWD)
- Mainlined open-source Linux® STM32 MPU OpenSTLinux Distribution and
  STM32CubeMP2 software with examples
- Linux® Yocto project, Buildroot, and STM32CubeIDE as development environments

More information about the board can be found at the
`STM32MP257F-EV1 website`_.

Hardware
********

Core:
=====

- STM32MP257FAI3 SoC with Cortex®-A35 dual-core and Cortex®-M33
- Hardware cryptography and secure boot support

Memories:
=========

- 2 x 16-Gbit DDR4 DRAMs (4 Gbytes in total)
- 512-Mbit (64 Mbytes) S-NOR flash memory
- 32-Gbit (4 Gbytes) eMMC v5.0
- microSD™ card socket (bootable)

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

- Cortex®-M33 with TrustZone®
- Cortex®-M0+ for secure boot
- Secure boot via ROM code and OTP configuration
- Tamper detection and VBAT RTC domain

Connectivity
============

- 3x Gigabit Ethernet (RGMII, TSN switch capable)
- 2x CAN FD
- USB 2.0 High-Speed Host (dual-port)
- USB Type-C® DRP
- mikroBUS™ expansion
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
------------

The Cortex®-M33 Core is configured to run at a 400 MHz clock speed.

Programming and Debugging
*************************

The STM32MP257F-EV1 Evaluation board has two different ways to load the
firmware on the Cortex®-M33 core, either from the SRAM or from the DDR, which
needs to be started by the cortex-A35.

The cortex-A35 is responsible for loading the binary to the correct memory
location and starting the firmware. This is done using the 'remoteproc'
framework. Remoteproc checks the different memory sections in the ELF file
looking for the vector table.

By default, it expects the section name to be '.isr_vectors'. In zephyr, the
vector table is located in the 'rom_start' section. Two possible ways to
properly load the firmware are available:
- Patching the remoteproc driver to look for the 'rom_start' section instead of
'.isr_vectors'. This is done by modifying the flashed image on the cortex-A35
side.
- Renaming the 'rom_start' section to '.isr_vectors' in the zephyr build
system. *This is the solution used for now.* The section name is changed in the
linker script located in :zephyr_file:`soc/st/stm32/stm32mp2x/m33/linker.ld`

Loading the firmware
====================
In the case of the MP boards, the linux part is responsible (in the current
distribution) of starting the Cortex-M33 core and loading the firmware using
remoteproc. The application can be built using west, taking the
:zephyr:code-sample:`blinky` as an example.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: stm32mp257f_ev1/stm32mp257fxx/m33
   :goals: build

The firmware can be copied to the board and started with remoteproc.
(more information about the procedure can be found in the
`STM32MP257F boot Cortex-M33 firmware`_)

Debugging
=========
Applications can be debugged using OpenOCD and GDB. The firmware must first be
started by the Cortex®-A35. The debugger can then be attached to the running
Zephyr firmware using OpenOCD.

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
  https://www.st.com/en/evaluation-tools/stm32mp257f-ev1.html

.. _STM32MP257F reference manual:
  https://www.st.com/resource/en/reference_manual/rm0457-stm32mp25xx-advanced-armbased-3264bit-mpus-stmicroelectronics.pdf

.. _STM32MP257F boot Cortex-M33 firmware:
  https://wiki.st.com/stm32mpu/wiki/Linux_remoteproc_framework_overview#Remote_processor_boot_through_sysfs
