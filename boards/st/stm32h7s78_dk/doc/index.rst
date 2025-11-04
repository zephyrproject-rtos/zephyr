.. zephyr:board:: stm32h7s78_dk

Overview
********

The STM32H7S78-DK Discovery kit is designed as a complete demonstration and
development platform for STMicroelectronics Arm |reg| Cortex |reg|-M7 core-based
STM32H7S7L8H6H microcontroller. Here are some highlights of the STM32H7S78-DK Discovery board:


- STM32H7S7L8H6H microcontroller featuring 64Kbytes of Flash memory and 620 Kbytes of SRAM in 225-pin TFBGA package
- USB Type-C |reg| Host and device with USB power-delivery controller
- SAI Audio DAC stereo with one audio jacks for input/output,
- ST MEMS digital microphone with PDM interface
- Octo-SPI interface connected to 512Mbit Octo-SPI NORFlash memory device (MX66UW1G45GXD100 from MACRONIX)
- 10/100-Mbit Ethernet,

- Board connectors

  - STMod+ expansion connector with fan-out expansion board for Wi‑Fi |reg|, Grove and mikroBUS |trade| compatible connectors
  - Pmod |trade| expansion connector
  - Audio MEMS daughterboard expansion connector
  - ARDUINO |reg| Uno V3 expansion connector

- Flexible power-supply options

  - ST-LINK
  - USB VBUS
  - external sources

- On-board STLINK-V3E debugger/programmer with USB re-enumeration capability:

  - mass storage
  - Virtual COM port
  - debug port

- 4 user LEDs
- User and reset push-buttons

More information about the board can be found at the `STM32H7S78-DK Discovery website`_.

Hardware
********

The STM32H7S7xx devices are a high-performance microcontrollers family (STM32H7
Series) based on the high-performance Arm |reg| Cortex |reg|-M7 32-bit RISC core.
They operate at a frequency of up to 500 MHz.

- Core: ARM |reg| 32-bit Cortex |reg| -M7 CPU with FPU.
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
  - Internal low-power 32 kHz RC (LSI)( |plusminus| 5%)
  - Internal 4 MHz oscillator (CSI), trimmable by software
  - Internal 48 MHz (HSI48) with recovery system
  - 3 PLLs for system clock, USB, audio, ADC

- Power management

  - Embedded regulator (LDO) with three configurable range output to supply the digital circuitry
  - Embedded SMPS step-down converter

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
- Development support: serial wire debug (SWD), JTAG, Embedded Trace Macrocell |trade|


More information about STM32H7S7 can be found here:

- `STM32H7Sx on www.st.com`_
- `STM32H7Sx reference manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Zephyr board options
====================

The STM32HS7 is a SoC with Cortex-M7 architecture. Zephyr provides support
for building for Secure firmware.

The BOARD options are summarized below:

+----------------------+-----------------------------------------------+
|   BOARD              | Description                                   |
+======================+===============================================+
| stm32h7s78_dk        | For building Secure firmware                  |
+----------------------+-----------------------------------------------+

Connections and IOs
===================

STM32H7S78-DK Discovery Board has 12 GPIO controllers. These controllers are responsible for pin muxing,
input/output, pull-up, etc.

For more details please refer to `STM32H7S78-DK Discovery board User Manual`_.

Default Zephyr Peripheral Mapping:
----------------------------------

- USART_4 TX/RX : PD1/PD0 (VCP)
- USART_7 TX/RX : PE8/PE7  (Arduino USART7)
- USER_PB : PC13
- LD1 (green) : PO1
- LD2 (orange) : PO5
- LD3 (red) : PM2
- LD4 (blue) : PM3
- ADC1 channel 6 input : PF12
- USB OTG FS DM/DP : PM12/PM11
- XSPI1 NCS/DQS0/DQS1/CLK/IO: PO0/PO2/PO3/PO4/PP0..15
- I2C1 SCL/SDA: PB6/PB9

System Clock
------------

STM32H7S78-DK System Clock could be driven by internal or external oscillator,
as well as main PLL clock. By default System clock is driven by PLL clock at
500MHz, driven by 24MHz external oscillator (HSE).

Serial Port
-----------

STM32H7S78-DK Discovery board has 2 U(S)ARTs. The Zephyr console output is
assigned to USART4. Default settings are 115200 8N1.

USB
---

STM32H7S78-DK Discovery board has 2 USB Type-C connectors. Currently, only
USB port2 (FS) is supported.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

STM32H7S78-DK Discovery board includes an ST-LINK/V3E embedded debug tool interface.

The board is configured to be flashed using west `STM32CubeProgrammer`_ runner,
so its :ref:`installation <stm32cubeprog-flash-host-tools>` is required.

Application in SoC Flash
========================

Here is an example for the :zephyr:code-sample:`hello_world` application.

Connect the STM32H7S78-DK Discovery to your host computer using the USB port,
then run a serial host program to connect with your Nucleo board:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

Then build and flash the application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32h7s78_dk
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   Hello World! stm32h7s78_dk

If the application size is too big to fit in SoC Flash,
Zephyr :ref:`Code and Data Relocation <code_data_relocation>` can be used to relocate
the non-critical and big parts of the application to external Flash.

Debugging
---------

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32h7s78_dk
   :maybe-skip-config:
   :goals: debug

Application in External Flash
=============================

Because of the limited amount of SoC Flash (64KB), you may want to store the application
in external QSPI Flash instead, and run it from there. In that case, the MCUboot bootloader
is needed to chainload the application. A dedicated board variant, ``ext_flash_app``, was created
for this usecase.

:ref:`sysbuild` makes it possible to build and flash all necessary images needed to run a user application
from external Flash.

The following example shows how to build :zephyr:code-sample:`hello_world` with Sysbuild enabled:

.. zephyr-app-commands::
   :tool: west
   :zephyr-app: samples/hello_world
   :board: stm32h7s78_dk/stm32h7s7xx/ext_flash_app
   :goals: build
   :west-args: --sysbuild

By default, Sysbuild creates MCUboot and user application images.

For more information, refer to the :ref:`sysbuild` documentation.

Flashing
--------

Both MCUboot and user application images can be flashed by running:

.. code-block:: console

   west flash

You should see the following message in the serial host program:

.. code-block:: console

   *** Booting MCUboot v2.2.0-192-g96576b341ee1 ***
   *** Using Zephyr OS build v4.3.0-rc2-37-g6cc7bdb58a92 ***
   I: Starting bootloader
   I: Bootloader chainload address offset: 0x0
   I: Image version: v0.0.0
   I: Jumping to the first image slot
   *** Booting Zephyr OS build v4.3.0-rc2-37-g6cc7bdb58a92 ***
   Hello World! stm32h7s78_dk/stm32h7s7xx/ext_flash_app

To only flash the user application in the subsequent builds, Use:

.. code-block:: console

   west flash --domain hello_world

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

   west debugserver

Then, open another terminal (don't forget to activate Zephyr's environment) and execute:

.. code-block:: console

   west attach

By default, user application symbols are loaded. To debug MCUboot application,
launch:

.. code-block:: console

   west attach --domain mcuboot

.. _STM32H7S78-DK Discovery website:
   https://www.st.com/en/evaluation-tools/stm32h7s78-dk.html

.. _STM32H7S78-DK Discovery board User Manual:
   https://www.st.com/en/evaluation-tools/stm32h7s78-dk.html

.. _STM32H7Sx on www.st.com:
   https://www.st.com/en/evaluation-tools/stm32h7s78-dk.html

.. _STM32H7Sx reference manual:
   https://www.st.com/resource/en/reference_manual/rm0477-stm32h7rx7sx-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html

.. _MCUboot design:
   https://docs.mcuboot.com/design.html

.. _MCUboot Encryption:
   https://docs.mcuboot.com/encrypted_images.html
