.. zephyr:board:: stm32mp257f_dk

Overview
********

The STM32MP257F-DK Discovery kit is designed as a complete demonstration
and development platform for the STMicroelectronics STM32MP257F microprocessor
based on Arm |reg| dual-core Cortex |reg|-A35 (1.5 GHz) and Cortex |reg|-M33
(400 MHz), and the STPMIC25 companion chip.
Zephyr OS is ported to run on the Cortex |reg|-M33 core, as a coprocessor of
the Cortex |reg|-A35 core.

Features:
=========

- STM32MP257FAI3 microprocessor featuring dual-core Arm |reg| Cortex |reg|-A35,
  a Cortex |reg|-M33 in a VFBGA424 package
- ST power management STPMIC25
- 32‑Gbit LPDDR4 DRAM
- 64-Gbit eMMC v5.1
- 1‑Gbit/s Ethernet (RGMII)
- Two USB 2.0 high speed
- USB 3.0 SuperSpeed PD (DRP/DRD)
- Wi‑Fi® 802.11b/g/n
- Bluetooth® LE
- Four user LEDs
- Two user, one tamper, and one reset push-buttons
- Wake-up button
- Four boot pin switches
- Board connectors:

  - Ethernet RJ45
  - Two stacked USB 2.0 HS Type-A
  - USB 3.0 USB Type-C |reg| PD
  - microSD |trade| card holder
  - Dual-lane MIPI CSI-2 |reg| camera module expansion connector
  - HDMI |reg|
  - LVDS
  - GPIO expansion connector
  - VBAT for power backup

- On-board STLINK-V3EC:

  - Debugger with USB re-enumeration capability: Virtual COM port and debug port
  - Board power source through USB Type-C |reg|

- Mainlined open-source Linux |reg| STM32 MPU OpenSTLinux Distribution and
  STM32CubeMP2 software with examples
- Linux |reg| Yocto project |reg|, Buildroot, and STM32CubeIDE as
  development environments

More information about the board can be found at the
`STM32MP257F-DK website`_.

Hardware
********

- Cores

  - 64-bit dual-core Arm |reg| Cortex |reg|-A35 with 1.5 GHz max frequency
  - 32-bit Arm |reg| Cortex |reg|-M33 with FPU/MPU, Arm |reg| TrustZone |reg|,  and 400 MHz max frequency

- Memories

  - External DDR memory up to 4 Gbytes
  - 808-Kbyte internal SRAM
  - Two octo-spi memory interfaces
  - Flexible external memory controller with up to 16-bit data bus

- Power

  - STPMIC25 for voltage regulation (multiple buck/LDO regulators)
  - USB-C power input

- Clock management

  - 32.768 kHz LSE crystal
  - 40 MHz HSE crystal
  - Internal 64 MHz HSI oscillator
  - Internal 4 MHz CSI oscillator
  - Internal 32 kHz LSI oscillator
  - Five separate PLLs with integer and fractional mode

- Security/Safety

  - Secure boot, TrustZone |reg| peripherals, active tamper, environmental
    monitors, display secure layers, hardware accelerators
  - Complete resource isolation framework

- Connectivity

  - 1x Gigabit Ethernet (RGMII)
  - USB 2.0 High-Speed Host (dual-port)
  - USB Type-C |reg| DRP
  - GPIO expansion connector

- Display & Camera

  - LVDS interface (4-lane)
  - Camera CSI-2 interface (2-lane)

- Debug

  - STLINK-V3EC (onboard debugger with VCP, JTAG and SWD)

More information about STM32MP257F can be found here:

- `STM32MP257F on www.st.com`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

STM32MP257F-DK Discovery kit schematic is available here:
`STM32MP257F-DK Discovery kit schematics`_

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

Before you can run Zephyr on the STM32MP257F-DK Discovery kit, you need to
set up the Cortex |reg|-A35 core with a Linux |reg| environment. The Cortex
|reg|-M33 core runs Zephyr as a coprocessor, and it requires the Cortex
|reg|-A35 to load and start the firmware using remoteproc.

One way to set up the Linux environment is to use the official ST
OpenSTLinux distribution, following the `Starter Package`_. (more information
about the procedure can be found in the `STM32MPU Wiki`_)

Loading the firmware
====================

Once the OpenSTLinux distribution is installed on the board, the Cortex |reg|
-A35 is responsible (in the current distribution) for loading the Zephyr
firmware image in DDR and/or SRAM and starting the Cortex |reg| -M33 core. The
application can be built using west, taking the :zephyr:code-sample:`blinky` as
an example.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: stm32mp257f_dk/stm32mp257fxx/m33
   :goals: build

The firmware can be copied to the board file system and started with the Linux
remoteproc framework. (more information about the procedure can be found in the
`STM32MP257F boot Cortex-M33 firmware`_)

Debugging
=========
Applications can be debugged using OpenOCD and GDB. The OpenOCD files can be
found at `device-stm-openocd`_.
The firmware must first be started by the Cortex |reg|-A35. The debugger can
then be attached to the running Zephyr firmware using OpenOCD.

- Build the sample:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: stm32mp257f_dk/stm32mp257fxx/m33
   :goals: build

- Copy the firmware to the board, load it and start it with remoteproc
  (`STM32MP257F boot Cortex-M33 firmware`_). The orange LED should be blinking.
- Attach to the target:

.. code-block:: console

   $ west attach

References
==========

.. target-notes::

.. _STM32MP257F-DK website:
  https://www.st.com/en/evaluation-tools/stm32mp257f-dk.html#overview

.. _STM32MP257F-DK Discovery kit User Manual:
  https://www.st.com/resource/en/user_manual/um3385-discovery-kit-with-stm32mp257f-mpu-stmicroelectronics.pdf

.. _STM32MP257F-DK Discovery kit schematics:
  https://www.st.com/resource/en/schematic_pack/mb1605-mp257f-c01-schematic.pdf

.. _STM32MP25xC/F Discovery kit datasheet:
  https://www.st.com/resource/en/datasheet/stm32mp257c.pdf

.. _STM32MP257F on www.st.com:
  https://www.st.com/en/microcontrollers-microprocessors/stm32mp257f.html

.. _STM32MP257F reference manual:
  https://www.st.com/resource/en/reference_manual/rm0457-stm32mp25xx-advanced-armbased-3264bit-mpus-stmicroelectronics.pdf

.. _STM32MP257F boot Cortex-M33 firmware:
  https://wiki.st.com/stm32mpu/wiki/Linux_remoteproc_framework_overview#Remote_processor_boot_through_sysfs

.. _Starter Package:
  https://wiki.stmicroelectronics.cn/stm32mpu/wiki/STM32MP25_Discovery_kits_-_Starter_Package

.. _STM32MPU Wiki:
  https://wiki.st.com/stm32mpu/wiki/Main_Page

.. _device-stm-openocd:
  https://github.com/STMicroelectronics/device-stm-openocd/tree/main
