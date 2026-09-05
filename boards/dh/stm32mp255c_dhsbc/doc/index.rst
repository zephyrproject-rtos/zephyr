.. zephyr:board:: stm32mp255c_dhsbc

Overview
********

The DHSBC STM32MP255C is an industrial-grade, ready-to-use single board computer
that has been specially developed for IoT and Human Machine Interface (HMI)
applications. It is based on the solderable, pin-compatible DHCOS STM32MP2
System on Module, which enables a modular and scalable system architecture.
The platform is designed for high performance, long-term availability of over
ten years and use in demanding industrial environments.

With full support for mainline Linux, including the Vivante GPU,
the DHSBC STM32MP255C provides a future-proof software base for the development
of graphical user interfaces and complex applications. Integrated security
functions such as Secure Boot and Secure Storage ensure the protection of
sensitive data and enable the implementation of "Security by Design" approaches.
Connectivity options range from Gbit-Ethernet, WiFi and Bluetooth to advanced
multimedia functions such as 3D GPU, display, camera and video support.
Thanks to the integrated STPMIC25 power management chip, the DHSBC STM32MP255C
is also suitable for applications with high demands on energy efficiency and
low-power modes.

Zephyr OS is ported to run on the Cortex®-M33 core as a coprocessor of the
Cortex®-A35 cores, enabling real-time and low-power applications alongside
Linux-based high-performance processing on the application cores.

Features:
=========

- STM32MP255CAK3 microprocessor featuring dual-core Arm® Cortex®-A35,
  a Cortex®-M33 and a Cortex®-M0+ in a VFBGA424 package
- ST power management STPMIC25DPQR
- 4 GB DRAM (LPDDR4-2400 32-bit)
- 16 GB eMMC flash
- 4 MB SPI NOR flash
- 4 kB EEPROM
- Two 1-Gbit/s Ethernet (RGMII)
- High-speed USB Host hub
- High-speed USB Type-C 3.2 Gen 1x1 with DisplayPort alt. mode support
- Bluetooth® v5.4 BR/EDR/LE
- WiFi (Tri band 2.4 GHz, 5 GHz and 6 GHz for IEEE802.11a/b/g/n/ac/ax)
- Power and Reset buttons
- Four boot pin switches
- Board connectors:

  - Two Ethernet RJ45
  - One USB Host Type-A
  - USB Type-C® (data)
  - USB Type-C® (power supply)
  - microSD™ card holder
  - Dual-lane MIPI CSI-2® camera module expansion connector
  - LVDS
  - Three-pin UART connector (serial console)
  - Raspberry Pi 40-pin expansion connector
  - VBAT for RTC and backup SRAM
  - JTAG

- Linux® Yocto project BSP

More information about the board and SoC can be found at the
`STM32MP255C-DHSBC website`_, `STM32MP255C website`_ and the
`STM32MP255C reference manual`_

Hardware
********

Cores:
======

- 64-bit dual-core Arm® Cortex®-A35 with 1.2 GHz max frequency

  - 32-Kbyte I + 32-Kbyte D level 1 cache for each Cortex®-A35 core
  - 512-Kbyte unified level 2 cache
  - Arm® NEON™ and Arm® TrustZone®

- 32-bit Arm® Cortex®-M33 with FPU/MPU, Arm® TrustZone®,
  and 400 MHz max frequency

  - L1 16-Kbyte ICache / 16-Kbyte DCache for Cortex®-M33

- 32-bit Arm® Cortex®-M0+ in SmartRun domain with 200 MHz max
  frequency (up to 16 MHz in autonomous mode)

Memories:
=========

- External DDR memory 4 Gbytes (LPDDR4-2400 32-bit)
- 808-Kbyte internal SRAM: 256-Kbyte AXI SYSRAM, 128-Kbyte AXI video RAM or
  SYSRAM extension, 256-Kbyte AHB SRAM, 128-Kbyte AHB SRAM with ECC in backup
  domain, 8-Kbyte SRAM with ECC in backup domain, 32 Kbytes in SmartRun domain
- Two Octo-SPI memory interfaces
- Flexible external memory controller with up to 16-bit data bus: parallel
  interface to connect external ICs, and SLC NAND memories with up to 8-bit ECC

Power
=====

- STPMIC25 for voltage regulation (multiple buck/LDO regulators)
- USB-C for power input
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

- Secure boot, TrustZone® peripherals, active tamper, environmental
  monitors, display secure layers, hardware accelerators
- Complete resource isolation framework

Connectivity
============

- 2x Gigabit Ethernet (RGMII)
- USB 2.0 High-Speed Host
- USB Type-C®
- Raspberry Pi 40-pin expansion connector

Display & Camera
================

- LVDS interface (1x Dual Link, 2x 4-lane LVDS connector)
- MIPI-CSI2 1x 2-lanes connector

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

For connections and IOs see the quick start guide of the
DHSBC STM32MP255C board:
`STM32MP255C-DHSBC quick start guide`_

System Clock
============

Cortex®-A35
------------

Not yet supported in Zephyr.

Cortex®-M33
-----------

The Cortex®-M33 Core is configured to run at a 400 MHz clock speed.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Prerequisite
============

Before you can run Zephyr on the DHSBC STM32MP255C, you need to
set up the Cortex®-A35 core with a Linux® environment. The Cortex®-M33 core runs
Zephyr as a coprocessor, and it requires the Cortex®-A35 to load and start the
firmware using remoteproc.

One way to set up the Linux environment is to use the DH electronics KAS/Yocto
Repository to build a Linux image: `STM32MP255C-DHSBC kas yocto`_ .
There are also pre-built images: `STM32MP255C-DHSBC prebuild images`_.

Another way is to use the OpenSTLinux distribution, following the Starter
Package 5. (more information about the procedure can be found in the
`STM32MPU Wiki`_)

Loading the firmware
====================

Once the Linux distribution is installed on the board, the Cortex®
-A35 is responsible for loading the Zephyr firmware image in DDR and/or SRAM
and starting the Cortex®-M33 core. The application can be built using west,
taking the :zephyr:code-sample:`hello_world` as an example.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32mp255c_dhsbc/stm32mp255cxx/m33
   :goals: build

The firmware can be copied to the board file system and started with the Linux
remoteproc framework.

To start the firmware:

.. code-block:: console

   $ cp zephyr.elf /lib/firmware/
   $ echo -n zephyr.elf > /sys/class/remoteproc/remoteproc0/firmware
   $ echo start > /sys/class/remoteproc/remoteproc0/state

To stop the firmware:

.. code-block:: console

   $ echo stop > /sys/class/remoteproc/remoteproc0/state

More information about the procedure can be found in the
`STM32MP257F boot Cortex-M33 firmware`_ ST Wiki page.

Debugging
=========

Applications can be debugged using a J-Link with the J-Link Software Pack or
with a ST-Link with OpenOCD. The default is J-Link.

For ST-Link the newest OpenOCD version of ST must be used, the files can be
found at `device-stm-openocd`_. To use this version use the ``--openocd`` and
``--openocd-search`` flags with ``west attach``.

The firmware must first be started by the Cortex®-A35. The debugger can
then be attached to the running Zephyr firmware.

- Build the sample

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/uart/echo_bot
   :board: stm32mp255c_dhsbc/stm32mp255cxx/m33
   :goals: build

- Copy the firmware to the board, load it and start it with remoteproc
  (`STM32MP257F boot Cortex-M33 firmware`_). It will echo back all bytes on
  ``raspberry_pi_serial``.

- Attach to the target with J-Link:

.. code-block:: console

   $ west attach

- Alternatively attach to the target with ST-Link and OpenOCD (it is assumed
  ST's OpenOCD was downloaded to ``/opt/device-stm-openocd``):

.. code-block:: console

   $ west attach --runner openocd \
     --openocd "/opt/device-stm-openocd/prebuilt/openocd" \
     --openocd-search "/opt/device-stm-openocd/prebuilt/scripts/"


References
==========

.. target-notes::

.. _STM32MP255C-DHSBC website:
  https://www.dh-electronics.com/en/embedded-products/development-carrier-boards/detail/dhsbc-stm32mp255c

.. _STM32MP255C-DHSBC quick start guide:
  https://wiki.dh-electronics.com/images/e/ef/DOC_DHSBC-STM32MP25-Quick-Start-Guide_R01_2025-09-11.pdf

.. _STM32MP255C-DHSBC kas yocto:
  https://github.com/dh-electronics/kas-dhsom

.. _STM32MP255C-DHSBC prebuild images:
  https://github.com/dh-electronics/prebuilt-test-images-dhsom

.. _STM32MP255C website:
  https://www.st.com/en/microcontrollers-microprocessors/stm32mp255c.html

.. _STM32MP255C reference manual:
  https://www.st.com/resource/en/reference_manual/rm0457-stm32mp25xx-advanced-armbased-3264bit-mpus-stmicroelectronics.pdf

.. _STM32MP257F boot Cortex-M33 firmware:
  https://wiki.st.com/stm32mpu/wiki/Linux_remoteproc_framework_overview#Remote_processor_boot_through_sysfs

.. _STM32MPU Wiki:
  https://wiki.st.com/stm32mpu/wiki/Main_Page

.. _device-stm-openocd:
  https://github.com/STMicroelectronics/device-stm-openocd/tree/main
