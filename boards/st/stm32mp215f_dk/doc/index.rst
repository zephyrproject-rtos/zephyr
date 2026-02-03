.. zephyr:board:: stm32mp215f_dk

Overview
********

The STM32MP215F-DK Discovery kit is designed as a complete demonstration
and development platform for the STMicroelectronics STM32MP215F microprocessor
based on the Arm |reg| Cortex |reg|-A35 (1.5 GHz) and Cortex |reg|-M33
processors.
Zephyr OS is ported to run on the Cortex |reg|-M33 core, as a coprocessor of
the Cortex |reg|-A35 core.

Features:
=========

- STM32MP215FAN3 microprocessor based on the Arm® Cortex |reg|-35 up to 1.5 GHz
  and Cortex |reg|-M33 at 300 MHz in a VFBGA273 package
- STMicroelectronics power management STPMIC2L
- 16-Gbit LPDDR4 DRAM
- 100-Mbit/s Ethernet (RMII)
- USB 2.0 Bus Powered
- Four user LEDs
- Two user, one tamper, and one reset push-buttons
- Wake-up button
- Four boot pin switches
- Board connectors:

  - Ethernet RJ45
  - USB 2.0 USB Type-C®
  - microSD™ card holder
  - Dual-lane MIPI CSI-2® camera module expansion connector
  - LTDC display connector
  - M.2 E-Key connector to support Wi-Fi® and Bluetooth® SDIO modules
  - GPIO expansion connector
  - VBAT for power backup
  - MIPI10 JTAG connector
  - STDC14 connector for debug

- Mainlined open-source Linux |reg| STM32 MPU OpenSTLinux Distribution and
  STM32CubeMP2 software with examples
- Linux |reg| Yocto Project |reg|, Buildroot, and STM32CubeIDE as
  development environments

More information about the board can be found at the
`STM32MP215F-DK website`_.


Hardware
********

Cores:
======
- 64-bit single-core Arm® Cortex®-A35
  - Up to 1.5 GHz
  - 32-Kbyte I + 32-Kbyte D level 1 cache for each core
  - 128-Kbyte level 2 cache
  - Arm® NEON™ and Arm® TrustZone®
- 32-bit Arm® Cortex®-M33 with FPU/MPU
  - Up to 300 MHz
  - L1 16-Kbyte I / 16-Kbyte D
  - Arm® TrustZone®


Memories:
=========
- External DDR memory up to 4 Gbytes
  - DDR3L-1600 16-bit
  - DDR4-1600 16-bit
  - LPDDR4-1600 16-bit
- 456-Kbyte internal SRAM: 256-Kbyte AXI SYSRAM, 64-Kbyte AHB SRAM,
128-Kbyte AHB SRAM with ECC in backup domain, 8-Kbyte SRAM with ECC
in backup domain
- One Octo-SPI memory interfaces
- Flexible external memory controller with up to 16-bit data bus: parallel
interface to connect external ICs, and SLC NAND memories with up to 8-bit ECC


Security/safety:
================
- Secure boot, TrustZone® peripherals, active tamper, environmental monitors,
display secure layer, hardware accelerators
- Complete resource isolation framework


Reset and power management:
===========================
- 1.71 to 1.95 V and 2.7/3.0 to 3.6 V multiple section I/O supply
- POR, PDR, PVD, and BOR
- On-chip LDO and power-switches for RETRAM, BKPSRAM, and VSW
- Dedicated supplies for Cortex®-A35
- Internal temperature sensor
- Low-power modes: Sleep, Stop, and Standby
- DDR memory retention in Standby mode
- Controls for PMIC companion chip



Clock management:
=================

- Internal oscillators: 64 MHz HSI, 16 MHz MSI, 32 kHz LSI
- External oscillators: 16-48 MHz HSE, 32.768 kHz LSE
- Up to 7× PLLs with fractional mode
General-purpose inputs/outputs
- Up to 123 secure I/O ports with interrupt capability
- Up to six wake-up inputs
- Up to seven tamper input pins + 5 active tampers output pins
Interconnect matrix
- Bus matrices
- 128-, 64-, 32-bit STNoC interconnect, up to 600 MHz
- 32-bit Arm® AMBA® AHB interconnect, up to 300 MHz
3 DMA controllers to unload the CPU
- 48 physical channels in total
- 3× dual master port, high-performance, general-purpose, direct memory access controller (HPDMA), 16
channels each


Up to 37 communication peripherals:
===================================

- 3× I2C FM+ (1 Mbit/s, SMBus/PMBus®)
- 3× I3C (12.5 Mbit/s)
- 3× UART + 4× USART (12.5 Mbit/s, ISO7816 interface, LIN, IrDA, SPI) + 1× LPUART
- 6× SPI (50 Mbit/s, including 3 with full duplex I2S audio class accuracy via internal audio PLL or external
clock)(+1 with OCTOSPI + 4 with USART)
- 4× SAI (stereo audio: I2S, PDM, SPDIF Tx)
- SPDIF Rx with four inputs
- 3× SDMMC up to 8-bit (SD/e•MMC™/SDIO)
- Up to 2× CAN controllers supporting CAN FD protocol, out of which one supports time-triggered CAN
(TTCAN)
- 1× USB 2.0 high-speed Host with embedded 480 Mbits/s PHY
- 1× USB 2.0 high-speed dual role data with embedded 480 Mbits/s PHY
- Up to 2× Gigabit Ethernet interfaces
- TSN, IEEE 1588v2 hardware, MII/RMII/RGMII
- Camera interface #1 (5 Mpixels at 30 fps)
- MIPI CSI-2®, 2× data lanes up to 2.5 Gbit/s each
- 8- to 16-bit parallel, up to 120 MHz
- RGB, YUV, JPG, RawBayer with light ISP
- Lite-ISP, demosaicing, downscaling, cropping, 3 pixel pipelines
- Camera interface #2 (1 Mpixels at 15 fps)
- 8- to 14-bit parallel, up to 80 MHz
- RGB, YUV, JPG
- Cropping
- Digital parallel interface up to 16-bit input or output


5 analog peripherals:
=====================

- 2 × ADCs with 12-bit max. resolution (up to 5 Msps each, up to 23 channels)
- Internal temperature sensor (DTS)
- 1× multifunction digital filter (MDF) with up to 4 channels/4 filters
- Internal (VREFBUF) or external ADC reference VREF+


Graphics:
=========

- LCD-TFT controller, up to 24-bit // RGB888
- Up to FHD (1920 × 1080) at 60 fps
- 3 layers including a secure layer
- YUV support


Up to 28 timers and 5 watchdogs:
================================

- 4× 32-bit timers with up to 4 IC/OC/PWM or pulse counter and quadrature (incremental) encoder input
- 2× 16-bit advanced motor control timers
- 10× 16-bit general-purpose timers (including 2 basic timers without PWM)
- 5× 16-bit low-power timers
- Secure RTC with subsecond accuracy and hardware calendar
- 4 Cortex®-A35 system timers (secure, nonsecure, virtual, hypervisor)
- 2× SysTick Cortex®-M33 timer (secure, nonsecure)
- 5× watchdogs (4× independent and 1× window)


Hardware acceleration:
======================

- 2x cryptographic processors (CRYP), AES-128, -192, -256, DES/TDES
- Secure AES-256 with SCA
- RSA, ECC, ECDSA with SCA
- 2x HASH (SHA-1, SHA-224, SHA-256, SHA3), HMAC
- 2x true random number generator
- CRC calculation unit
- “On-the-fly” DDR encryption/decryption (AES-128, AES-256)
- “On-the-fly” OTFDEC Octo-SPI flash memory decryption (AES-128)


Debug mode:
===========

- Arm® CoreSight™ trace and debug: SWD and JTAG interfaces



More information about STM32MP215F can be found here:

- `STM32MP215F on www.st.com`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

STM32MP215F-DK Evaluation Board schematic is available here:
`STM32MP215F-DK Evaluation board schematics`_

System Clock
============

Cortex |reg|-A35
----------------

Not yet supported in Zephyr.

Cortex |reg|-M33
----------------

The Cortex |reg|-M33 Core is configured to run at a 300 MHz clock speed.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Prerequisite
============

Before you can run Zephyr on the STM32MP215F-DK Discovery kit, you need to
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
   :board: stm32mp215f_dk/stm32mp215fxx/m33
   :goals: build

The firmware can be copied to the board file system and started with the Linux
remoteproc framework. (more information about the procedure can be found in the
`STM32MP215F boot Cortex-M33 firmware`_)

Debugging
=========
Applications can be debugged using OpenOCD and GDB. The OpenOCD files can be
found at `device-stm-openocd`_.
The firmware must first be started by the Cortex |reg|-A35. The debugger can
then be attached to the running Zephyr firmware using OpenOCD.

- Build the sample:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: stm32mp215f_dk/stm32mp215fxx/m33
   :goals: build

- Copy the firmware to the board, load it and start it with remoteproc
  (`STM32MP215F boot Cortex-M33 firmware`_). The orange LED should be blinking.
- Attach to the target:

.. code-block:: console

   $ west attach

References
==========

.. target-notes::

.. _STM32MP215F-DK website:
  https://www.st.com/en/evaluation-tools/stm32mp215f-dk.html#overview

.. _STM32MP215F-DK Discovery board User Manual:
  https://www.st.com/resource/en/user_manual/um3359-discovery-board-with-stm32mp215f-mpu-stmicroelectronics.pdf

.. _STM32MP215F-DK Discovery board schematics:
  https://www.st.com/resource/en/schematic_pack/mb1936-mp257f-x-d01-schematic.pdf

.. _STM32MP21xC/F Discovery board datasheet:
  https://www.st.com/resource/en/datasheet/stm32mp257c.pdf

.. _STM32MP215F on www.st.com:
  https://www.st.com/en/microcontrollers-microprocessors/stm32mp215f.html

.. _STM32MP215F reference manual:
  https://www.st.com/resource/en/reference_manual/rm0457-stm32mp25xx-advanced-armbased-3264bit-mpus-stmicroelectronics.pdf

.. _STM32MP215F boot Cortex-M33 firmware:
  https://wiki.st.com/stm32mpu/wiki/Linux_remoteproc_framework_overview#Remote_processor_boot_through_sysfs

.. _Starter Package:
  https://wiki.st.com/stm32mpu/wiki/STM32MP21_Discovery_kits_-_Starter_Package

.. _STM32MPU Wiki:
  https://wiki.st.com/stm32mpu/wiki/Main_Page

.. _device-stm-openocd:
  https://github.com/STMicroelectronics/device-stm-openocd/tree/main
