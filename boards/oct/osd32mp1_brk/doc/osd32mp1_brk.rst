.. zephyr:board:: osd32mp1_brk

Overview
********

The OSD32MP1-BRK development board by Octavo Systems integrates the OSD32MP15x
System-in-Package (SiP), which contains a multicore STM32MP157F microprocessor.
Zephyr OS is ported to run on the Cortex®-M4 core of the STM32MP157F.

- Common features:

  - OSD32MP15x SiP:

    - STM32MP15x microprocessor:
      - Dual-core Arm® Cortex®-A7 up to 800 MHz, 32 bits
      - Cortex®-M4 up to 209 MHz, 32 bits
      - Embedded SRAM (448 Kbytes) for Cortex®-M4.

    - 512MB DDR3 memory
    - STPMIC1A Power Management
    - Integrated 4kB EEPROM
    - MEMS oscillator
    - Over 100 passive components

  - Small form factor:
    - Dimensions: 75 mm x 46 mm (3 in x 1.8 in)
    - Breadboard-compatible with access to 106 I/Os via two 2x30 100-mil headers

  - Built-in features:
    - μUSB
    - ST-Link header
    - UART
    - μSD card slot
    - 32 kHz crystal
    - User LEDs and reset button
    - 4 Layer Design
    - No Back Side Components

For a detailed list of features, visit the `OSD32MP1-BRK product page`_.

Hardware
********

The OSD32MP15x SiP in integration with the STM32MP17 SoC provides the following hardware capabilities:

- Core:

  - 32-bit dual-core Arm® Cortex®-A7

    - L1 32-Kbyte I / 32-Kbyte D for each core
    - 256-Kbyte unified level 2 cache
    - Arm® NEON™ and Arm® TrustZone®

  - 32-bit Arm® Cortex®-M4 with FPU/MPU

    - Up to 209 MHz (Up to 703 CoreMark®)

- Memories:

  - 512 MB DDR3L memory (on SiP)
  - 708 Kbytes of internal SRAM:
    - 256 KB AXI SYSRAM
    - 384 KB AHB SRAM
    - 64 KB AHB SRAM in backup domain
  - Dual mode Quad-SPI memory interface
  - Flexible external memory controller with up to 16-bit data bus
  - Integrated 4 KB EEPROM (on SiP)

- Security/safety:

  - Secure boot, TrustZone® peripherals with Cortex®-M4 resource isolation

- Clock management:

  - Internal oscillators:
    - 64 MHz HSI oscillator
    - 4 MHz CSI oscillator
    - 32 kHz LSI oscillator
  - External oscillators:
    - 8-48 MHz HSE oscillator
    - 32.768 kHz LSE oscillator
  - 6 × PLLs with fractional mode
  - MEMS oscillator (on SiP)

- General-purpose input/outputs:

  - Up to 176 I/O ports with interrupt capability
  - 106 I/Os routed to expansion headers (on board)

- Interconnect matrix

- 3 DMA controllers

- Communication peripherals:

  - 6 × I2C FM+ (1 Mbit/s, SMBus/PMBus)
  - 4 × UART + 4 × USART (12.5 Mbit/s, ISO7816 interface, LIN, IrDA, SPI slave)
  - 6 × SPI (50 Mbit/s, including 3 with full duplex I2S audio class accuracy)
  - 4 × SAI (stereo audio: I2S, PDM, SPDIF Tx)
  - SPDIF Rx with 4 inputs
  - HDMI-CEC interface
  - MDIO Slave interface
  - 3 × SDMMC up to 8-bit (SD / e•MMC™ / SDIO)
  - 2 × CAN controllers supporting CAN FD protocol, TTCAN capability
  - 2 × USB 2.0 high-speed Host+ 1 × USB 2.0 full-speed OTG simultaneously
  - 10/100M or Gigabit Ethernet GMAC (IEEE 1588v2 hardware, MII/RMII/GMII/RGMI)
  - 8- to 14-bit camera interface up to 140 Mbyte/s
  - 6 analog peripherals
  - 2 × ADCs with 16-bit max. resolution
  - 1 × temperature sensor
  - 2 × 12-bit D/A converters (1 MHz)
  - 1 × digital filters for sigma delta modulator (DFSDM) with 8 channels/6
    filters
  - Internal or external ADC/DAC reference VREF+

- Graphics:

  - 3D GPU: Vivante® - OpenGL® ES 2.0
  - LCD-TFT controller, up to 24-bit // RGB888, up to WXGA (1366 × 768) @60 fps
  - MIPI® DSI 2 data lanes up to 1 GHz each

- Timers:

  - 2 × 32-bit timers with up to 4 IC/OC/PWM or pulse counter and quadrature
    (incremental) encoder input
  - 2 × 16-bit advanced motor control timers
  - 10 × 16-bit general-purpose timers (including 2 basic timers without PWM)
  - 5 × 16-bit low-power timers
  - RTC with sub-second accuracy and hardware calendar
  - 2 × 4 Cortex®-A7 system timers (secure, non-secure, virtual, hypervisor)
  - 1 × SysTick Cortex®-M4 timer

- Hardware acceleration:

  - AES 128, 192, 256, TDES
  - HASH (MD5, SHA-1, SHA224, SHA256), HMAC
  - 2 × true random number generator (3 oscillators each)
  - 2 × CRC calculation unit

- Debug mode:

  - Arm® CoreSight™ trace and debug: SWD and JTAG interfaces
  - 8-Kbyte embedded trace buffer
  - 3072-bit fuses including 96-bit unique ID, up to 1184-bit available for user

More information about the hardware can be found here:

- `STM32MP157F on www.st.com`_
- `OSD32MP15x SiP documentation`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

OSD32MP1-BRK Board schematic is available here:
`OSD32MP1-BRK schematics`_.

OSD32MP1-BRK Board pin mapping is available here:
`OSD32MP1-BRK default pin mapping`_.

Default Zephyr Peripheral Mapping:
----------------------------------

- UART7 TX/RX: PA15/PB3 (default console)
- I2C5 SCL/SDA: PA11/PA12
- SPI4 SCK/MISO/MOSI: PE12/PE13/PE14

System Clock
------------

The Cortex®-M4 Core is configured to run at a 209 MHz clock speed.
This value must match the configured mlhclk_ck frequency.

Serial Port
-----------

The Zephyr console output is assigned by default to the RAM console to be dumped
by the Linux Remoteproc Framework on Cortex®-A7 core. To enable the USART2 console, modify
the board's devicetree and the osd32mp1_brk_defconfig board file (or prj.conf project files)
Default USART settings are 115200 8N1.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The STM32MP157F doesn't have QSPI flash for Cortex®-M4 and it needs to be
started by the Cortex®-A7 core. The Cortex®-A7 core is responsible for loading the
Cortex®-M4 binary application into the RAM, and getting Cortex®-M4 out of reset.
Cortex®-A7 can perform these steps at bootloader level or after the Linux
system has booted.

Cortex®-M4 can use up to 2 different RAMs. The program pointer starts at
the 0x00000000 (RETRAM) address, and the vector table should be loaded at this address.
The following table provides memory mappings for Cortex®-A7 and Cortex®-M4:

+------------+-----------------------+------------------------+----------------+
| Region     | Cortex®-A7            | Cortex®-M4             | Size           |
+============+=======================+========================+================+
| RETRAM     | 0x38000000-0x3800FFFF | 0x00000000-0x0000FFFF  | 64KB           |
+------------+-----------------------+------------------------+----------------+
| MCUSRAM    | 0x10000000-0x1005FFFF | 0x10000000-0x1005FFFF  | 384KB          |
+------------+-----------------------+------------------------+----------------+
| DDR        | 0xC0000000-0x20000000 |                        | 512MB          |
+------------+-----------------------+------------------------+----------------+

Refer to following instructions to boot Zephyr on the Cortex®-M4 core:

1. Download and install the Octavo OpenSTLinux distribution:
   `OSD32MP1 OpenSTLinux`_.

   (You can find more details about this process here: `OSD32MP1-BRK Getting Started`_)

2. Build the Zephyr application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: osd32mp1_brk
      :goals: build

3. Transfer the built firmware to the board via USB RNDIS:

   .. code-block:: console

      scp build/zephyr/zephyr.elf root@192.168.7.1:/lib/firmware

4. Boot Zephyr on the Cortex®-M4 core:

   .. code-block:: console

      ssh root@192.168.7.1
      echo stop > /sys/class/remoteproc/remoteproc0/state
      echo -n zephyr.elf > /sys/class/remoteproc/remoteproc0/firmware
      echo start > /sys/class/remoteproc/remoteproc0/state
      cat /sys/kernel/debug/remoteproc/remoteproc0/trace0

   The console output should display:

   .. code-block::

      *** Booting Zephyr OS build v4.0.0 ***
      Hello World! osd32mp1_brk/osd32mp15x


Refer to `OSD32MP1-BRK Getting Started`_ and `stm32mp157 boot Cortex-M4 firmware`_ wiki page for more
detailed instructions.

Debugging
=========

You can debug an application using OpenOCD and GDB. The solution proposed below
is based on attaching to preloaded firmware, which is available only for a Linux
environment. The firmware must first be loaded by the Cortex®-A7. The developer
then attaches the debugger to the running Zephyr using OpenOCD.

The principle is to attach to the firmware already loaded by Linux.

- Build the sample:

  .. zephyr-app-commands::
     :zephyr-app: samples/hello_world
     :board: osd32mp1_brk
     :goals: build

- Copy the firmware on the target filesystem, load it and start it (`stm32mp157 boot Cortex-M4 firmware`_).
- Attach to the target:

  .. code-block:: console

    west attach

.. _OSD32MP1-BRK product page:
   https://octavosystems.com/octavo_products/osd32mp1-brk/

.. _OSD32MP1-BRK documentation:
   https://octavosystems.com/docs/osd32mp15x-datasheet/

.. _STM32MP157F on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32mp157f.html

.. _OSD32MP15x SiP documentation:
   https://octavosystems.com/docs/osd32mp15x-datasheet/

.. _OSD32MP1 OpenSTLinux:
   https://octavosystems.com/files/osd32mp1-brk-openstlinux-v3-0/

.. _OSD32MP1-BRK Getting Started:
    https://octavosystems.com/app_notes/osd32mp1-brk-getting-started/

.. _stm32mp157 boot Cortex-M4 firmware:
   https://wiki.st.com/stm32mpu/index.php/Linux_remoteproc_framework_overview#How_to_use_the_framework

.. _OSD32MP1-BRK schematics:
   https://octavosystems.com/docs/osd32mp1-brk-schematics/

.. _OSD32MP1-BRK default pin mapping:
   https://octavosystems.com/octavosystems.com/wp-content/uploads/2020/05/Default-Pin-Mapping.pdf
