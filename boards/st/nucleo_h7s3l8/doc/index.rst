.. zephyr:board:: nucleo_h7s3l8

Overview
********

The STM32 Nucleo-144 board provides an affordable and flexible way for users
to try out new concepts and build prototypes by choosing from the various combinations
of performance and power consumption features, provided by the STM32 microcontroller.

The ST Zio connector, which extends the ARDUINO® Uno V3 connectivity, and
the ST morpho headers provide an easy means of expanding the functionality of the Nucleo
open development platform with a wide choice of specialized shields.
The STM32 Nucleo-144 board does not require any separate probe as it integrates
the ST-LINK V3 debugger/programmer.

The STM32 Nucleo-144 board comes with the STM32 comprehensive free software
libraries and examples available with the STM32Cube MCU Package.

Key Features

- STM32 microcontroller with 64Kbytes of flash and 620Kbytes of RAM in TFBGA225 package
- Ethernet compliant with IEEE-802.3-2002
- USB USB Device only, USB OTG full speed, or SNK/UFP (full-speed or high-speed mode)
- 3 user LEDs
- 2 user and reset push-buttons
- 32.768 kHz crystal oscillator
- Board connectors:

 - USB with Micro-AB or USB Type-C®
 - Ethernet RJ45
 - MIPI20 compatible connector with trace signals

- Flexible power-supply options: ST-LINK USB VBUS or external sources
- External or internal SMPS to generate Vcore logic supply
- On-board ST-LINK/V3 debugger/programmer with USB re-enumeration
- capability: mass storage, virtual COM port and debug port

More information about the board can be found at the `Nucleo H7S3L8 website`_.

Hardware
********

Nucleo H7S3L8 provides the following hardware components:

The STM32H7S7xx devices are a high-performance microcontrollers family (STM32H7
Series) based on the high-performance Arm® Cortex®-M7 32-bit RISC core.
They operate at a frequency of up to 500 MHz.

- Core: ARM® 32-bit Cortex®-M7 CPU with FPU.
- Performance benchmark:

  - 1284 DMPIS/MHz (Dhrystone 2.1)

- Security

  - Flexible lifecycle scheme with secure debug authentication
  - Preconfigured immutable root of trust (ST-iROT)
  - SFI (secure firmware installation)
  - Secure data storage with hardware unique key (HUK)
  - Secure firmware upgrade support with TF-M
  - 2x AES coprocessors including one with DPA resistance
  - Public key accelerator, DPA resistant
  - On-the-fly decryption of Octo-SPI external memories
  - HASH hardware accelerator
  - 96-bit unique ID
  - Active tampers
  - True Random Number Generator (RNG) NIST SP800-90B compliant

- Clock management:

  - 24 MHz crystal oscillator (HSE)
  - 32768 Hz crystal oscillator for RTC (LSE)
  - Internal 64 MHz (HSI) trimmable by software
  - Internal low-power 32 kHz RC (LSI)(±5%)
  - Internal 4 MHz oscillator (CSI), trimmable by software
  - Internal 48 MHz (HSI48) with recovery system
  - 3 PLLs for system clock, USB, audio, ADC

- Power management

  - Embedded regulator (LDO) with three configurable range output to supply the digital circuitry


- RTC with HW calendar, alarms and calibration
- Up to 152 fast I/Os, most 5 V-tolerant, up to 10 I/Os with independent supply down to 1.08 V
- Up to 16 timers and 2 watchdogs

  - 16x 16-bit
  - 4x 32-bit timers with up to 4 IC/OC/PWM or pulse counter and quadrature (incremental) encoder input
  - 5x 16-bit low-power 16-bit timers (available in Stop mode)
  - 2x watchdogs
  - 1x SysTick timer

- Memories

  - Up to 64KB Flash, 2 banks read-while-write
  - 1 Kbyte OTP (one-time programmable)
  - 640 KB of SRAM including 64 KB with hardware parity check and 320 Kbytes with flexible ECC
  - 4 Kbytes of backup SRAM available in the lowest power modes
  - Flexible external memory controller with up to 16-bit data bus: SRAM, PSRAM, FRAM, SDRAM/LPSDR SDRAM, NOR/NAND memories
  - 2x OCTOSPI memory interface with on-the-fly decryption and support for serial PSRAM/NAND/NOR, Hyper RAM/Flash frame formats
  - 1x HEXASPI memory interface with on-the-fly decryption and support for serial PSRAM/NAND/NOR, Hyper RAM/Flash frame formats
  - 2x SD/SDIO/MMC interfaces

- Rich analog peripherals (independent supply)

  - 2x 12-bit ADC with up to 5 MSPS in 12-bit
  - 1x Digital temperature sensor

- 35x communication interfaces

  - 1x USB Type-C / USB power-delivery controller
  - 1x USB OTG full-speed with PHY
  - 1x USB OTG high-speed with PHY
  - 3x I2C FM+ interfaces (SMBus/PMBus)
  - 1x I3C interface
  - 7x U(S)ARTS (ISO7816 interface, LIN, IrDA, modem control)
  - 2x LP UART
  - 6x SPIs including 3 muxed with full-duplex I2S
  - 2x SAI
  - 2x FDCAN
  - 2x SD/SDIO/MMC interface
  - 2x 16 channel DMA controllers
  - 1x 8- to 16- bit camera interface
  - 1x HDMI-CEC
  - 1x Ethernel MAC interface with DMA controller
  - 1x 16-bit parallel slave synchronous-interface
  - 1x SPDIF-IN interface
  - 1x MDIO slave interface

- CORDIC for trigonometric functions acceleration
- FMAC (filter mathematical accelerator)
- CRC calculation unit
- Development support: serial wire debug (SWD), JTAG, Embedded Trace Macrocell™



More information about STM32H7S3 can be found here:

- `STM32H7S3L8 on www.st.com`_
- `STM32H7Sx reference manual`_


Supported Features
==================

.. zephyr:board-supported-hw::

For more details please refer to `STM32H7R/S Nucleo-144 board User Manual`_.

Default Zephyr Peripheral Mapping:
----------------------------------

The Nucleo H7S3L8 board features a ST Zio connector (extended Arduino Uno V3)
and a ST morpho connector. Board is configured as follows:

- UART_3 TX/RX : PD8/PD9 (ST-Link Virtual Port Com)
- USER_PB : PC13
- LD1 : PD10
- LD2 : PD13
- LD3 : PB7
- I2C : PB8, PB9
- SPI1 NSS/SCK/MISO/MOSI : PD14PA5/PA6/PB5 (Arduino SPI)
- FDCAN1 RX/TX : PD0, PD1
- ETH : A2, A7, B6, G4, G5, G6, G11, G12, G13

System Clock
------------

Nucleo H7S3L8 System Clock could be driven by an internal or external
oscillator, as well as the main PLL clock. By default, the System clock is
driven by the PLL clock at 600MHz, driven by an 24MHz high-speed external clock.

Serial Port
-----------

Nucleo H7S3L8 board has 4 UARTs and 3 USARTs plus one LowPower UART. The Zephyr console
output is assigned to UART3. Default settings are 115200 8N1.

Backup SRAM
-----------

In order to test backup SRAM you may want to disconnect VBAT from VDD. You can
do it by removing ``SB13`` jumper on the back side of the board.

FDCAN
-----

The Nucleo H7S3L8 board does not have any onboard CAN transceiver. In order to
use the FDCAN bus on this board, an external CAN bus transceiver must be
connected to pins PD0 (RX) and PD1 (TX).

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Nucleo H7S3L8 board includes an ST-LINK/V3 embedded debug tool interface.

.. note::

   Check if your ST-LINK V3 has newest FW version. It can be done with `STM32CubeProgrammer`_

Flashing
========

The board is configured to be flashed using west `STM32CubeProgrammer`_ runner,
so its :ref:`installation <stm32cubeprog-flash-host-tools>` is required.

Alternatively, OpenOCD or JLink can also be used to flash the board using
the ``--runner`` (or ``-r``) option:

.. code-block:: console

   $ west flash --runner openocd

Flashing an application to Nucleo H7S3L8
----------------------------------------

First, connect the NUCLEO-H7S3L8 to your host computer using
the USB port to prepare it for flashing. Then build and flash your application.

Here is an example for the :zephyr:code-sample:`hello_world` application.

Run a serial host program to connect with your NUCLEO-H7S3L8 board.

.. code-block:: console

   $ minicom -b 115200 -D /dev/ttyACM0

or use screen:

.. code-block:: console

   $ screen /dev/ttyACM0 115200

Build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucleo_h7s3l8
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   $ Hello World! nucleo_h7s3l8

Blinky example can also be used:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: nucleo_h7s3l8
   :goals: build flash

Debugging
---------

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucleo_h7s3l8
   :maybe-skip-config:
   :goals: debug

Application in External Flash
=============================

Because of the limited amount of SoC Flash (64KB), you may want to store the application
in external OSPI Flash instead, and run it from there. In that case, the MCUboot bootloader
is needed to chainload the application. A dedicate board variant, ``ext_flash_app``, was created
for this usecase.

:ref:`sysbuild` makes it possible to build and flash all necessary images needed to run a user application
from external Flash.

The following example shows how to build :zephyr:code-sample:`hello_world` with Sysbuild enabled:

.. zephyr-app-commands::
   :tool: west
   :zephyr-app: samples/hello_world
   :board: nucleo_h7s3l8/stm32h7s3xx/ext_flash_app
   :goals: build
   :west-args: --sysbuild

By default, Sysbuild creates MCUboot and user application images.

Build directory structure created by Sysbuild is different from traditional
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

   With ``--sysbuild`` option, MCUboot will be re-built every time the pristine build is used,
   but only needs to be flashed once if none of the MCUboot configs are changed.

For more information about the system build please read the :ref:`sysbuild` documentation.

Both MCUboot and user application images can be flashed by running:

.. code-block:: console

   $ west flash

You should see the following message in the serial host program:

.. code-block:: console

   *** Booting MCUboot v2.2.0-224-g0a52195c8181 ***
   *** Using Zephyr OS build v4.3.0-937-ge0490cf53e03 ***
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
   I: Bootloader chainload address offset: 0x0
   I: Image version: v0.0.0
   I: Jumping to the first image slot
   *** Booting Zephyr OS build v4.3.0-937-ge0490cf53e03 ***
   Hello World! nucleo_h7s3l8/stm32h7s3xx/ext_flash_app

To only flash the user application in the subsequent builds, Use:

.. code-block:: console

   $ west flash --domain hello_world

With the default configuration, the board uses MCUboot's Swap-using-offset mode.
To get more information about the different MCUboot operating modes and how to
perform application upgrade, refer to `MCUboot design`_.
To learn more about how to secure the application images stored in external Flash,
refer to `MCUboot Encryption`_.

Debugging
---------

You can debug the application in external flash using ``west`` and ``GDB``.

After flashing MCUboot and the app, execute the following command:

.. code-block:: console

   $ west debugserver

Then, open another terminal (don't forget to activate Zephyr's environment) and execute:

.. code-block:: console

   $ west attach

.. _Nucleo H7S3L8 website:
   https://www.st.com/en/evaluation-tools/nucleo-h7s3l8.html

.. _STM32H7R/S Nucleo-144 board User Manual:
   https://www.st.com/resource/en/user_manual/um3276-stm32h7rx7sx-nucleo144-board-mb1737-stmicroelectronics.pdf

.. _STM32H7S3L8 on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32h7s3l8.html

.. _STM32H7Sx reference manual:
   https://www.st.com/resource/en/reference_manual/rm0477-stm32h7rx7sx-armbased-32bit-mcus-stmicroelectronics.pdf

.. _OpenOCD installing Debug Version:
   https://github.com/zephyrproject-rtos/openocd

.. _OpenOCD installing with ST-LINK V3 support:
   https://mbd.kleier.net/integrating-st-link-v3.html

.. _STM32CubeIDE:
   https://www.st.com/en/development-tools/stm32cubeide.html

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html

.. _MCUboot design:
   https://docs.mcuboot.com/design.html

.. _MCUboot Encryption:
   https://docs.mcuboot.com/encrypted_images.html
