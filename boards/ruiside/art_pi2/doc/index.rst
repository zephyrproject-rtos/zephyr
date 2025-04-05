.. zephyr:board:: art_pi2

Overview
********

The ART-Pi2 is an open-source hardware platform designed by the
RT-Thread team specifically for embedded software engineers
and open-source makers, offering extensive expandability for DIY projects.

Key Features

- STM32H7R7L8HxH microcontroller featuring 64 Kbytes of Flash and 620 Kbytes of SRAM in an TFBGA225 package
- On-board ST-LINK/V2.1 debugger/programmer
- SDIO TF Card slot
- SDIO WIFI:CYWL6208
- HDC UART BuleTooth:CYWL6208
- 32-MB HyperRAM
- 64-MB HyperFlash
- One Power LED (blue) for 3.3 V power-on
- Two user LEDs blue and red
- Two ST-LINK LEDs: blue and red
- Two push-buttons (user and reset)
- Board connectors:

  - USB OTG with Type-C connector
  - RGB888 FPC connector

More information about the board can be found at the `ART-Pi2 website`_.

Hardware
********

ART-Pi2 provides the following hardware components:

The STM32H7R7xx devices are a high-performance microcontrollers family (STM32H7
Series) based on the high-performance Arm |reg| Cortex |reg|-M7 32-bit RISC core.
They operate at a frequency of up to 600 MHz.

- Core: ARM |reg| 32-bit Cortex |reg| -M7 CPU with TrustZone |reg| and FPU.
- Performance benchmark:

  - 1284 DMPIS/MHz (Dhrystone 2.1)

- Security

  - Arm |reg| TrustZone |reg| with ARMv8-M mainline security extension
  - Up to 8 configurable SAU regions
  - TrustZone |reg| aware and securable peripherals
  - Flexible lifecycle scheme with secure debug authentication
  - Preconfigured immutable root of trust (ST-iROT)
  - SFI (secure firmware installation)
  - Secure data storage with hardware unique key (HUK)
  - Secure firmware upgrade support with TF-M
  - 2x AES coprocessors including one with DPA resistance
  - Public key accelerator, DPA resistant
  - On-the-fly decryption of Octo-SPI external memories
  - HASH hardware accelerator
  - True random number generator, NIST SP800-90B compliant
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



More information about STM32H7R7 can be found here:

- `STM32H7R7L8 on www.st.com`_
- `STM32H7Rx reference manual`_


Supported Features
==================

.. zephyr:board-supported-hw::

Default Zephyr Peripheral Mapping:
----------------------------------

The ART-Pi2 board features a On-board ST-LINK/V2.1 debugger/programmer. Board is configured as follows:

- UART4 TX/RX : PD1/PD0 (ST-Link Virtual Port Com)
- LED1 (red) : PO1
- LED2 (blue) : PO5
- USER PUSH-BUTTON : PC13

System Clock
------------

ART-Pi2 System Clock could be driven by an internal or external
oscillator, as well as the main PLL clock. By default, the System clock is
driven by the PLL clock at 250MHz, driven by an 24MHz high-speed external clock.

Serial Port
-----------

ART-Pi2 board has 4 UARTs and 3 USARTs plus one LowPower UART. The Zephyr console
output is assigned to UART4. Default settings are 115200 8N1.

Backup SRAM
-----------

In order to test backup SRAM you may want to disconnect VBAT from VDD. You can
do it by removing ``SB13`` jumper on the back side of the board.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

ART-Pi2 board includes an ST-LINK/V2.1 embedded debug tool interface.

.. note::

   Check if your ST-LINK V2.1 has newest FW version. It can be done with `STM32CubeProgrammer`_

Flashing
========

The board is configured to be flashed using west `STM32CubeProgrammer`_ runner,
so its :ref:`installation <stm32cubeprog-flash-host-tools>` is required.

Flashing an application to ART-Pi2
----------------------------------

First, connect the art_pi2 to your host computer using
the USB port to prepare it for flashing. Then build and flash your application.

Here is an example for the :zephyr:code-sample:`hello_world` application.

Run a serial host program to connect with your art_pi2 board.

.. code-block:: console

   $ minicom -b 115200 -D /dev/ttyACM0

or use screen:

.. code-block:: console

   $ screen /dev/ttyACM0 115200

Build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: art_pi2
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   *** Booting Zephyr OS build v4.1.0-1907-g415ab379a8af ***
   Hello World! art_pi2/stm32h7r7xx

Blinky example can also be used:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: art_pi2
   :goals: build flash

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: art_pi2
   :maybe-skip-config:
   :goals: debug

.. _ART-Pi2 website:
   https://github.com/RT-Thread-Studio/sdk-bsp-stm32h7r-realthread-artpi2

.. _STM32H7R7L8 on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32h7r7l8.html

.. _STM32H7Rx reference manual:
   https://www.st.com/resource/en/reference_manual/rm0477-stm32h7rx7sx-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
