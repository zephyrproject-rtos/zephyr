.. zephyr:board:: nucleo_h533re

Overview
********

The Nucleo H533RE board is designed as an affordable development platform for
STMicroelectronics ARM |reg| Cortex |reg|-M33 core-based STM32H533RET6
microcontroller with TrustZone |reg|.
Here are some highlights of the Nucleo H533RE board:

- STM32H533RE microcontroller featuring 512 kbytes of Flash memory and 272 Kbytes of
  SRAM in LQFP64 package

- Board connectors:

  - USB Type-C |trade| Sink device FS
  - ST Zio expansion connector including Arduino Uno V3 connectivity (CN5, CN6, CN8, CN9)
  - ST morpho extension connector (CN7, CN10)

- Flexible board power supply:

   - 5V_USB_STLK from ST-Link USB connector
   - VIN (7 - 12V, 0.8) supplied via pin header CN6 pin 8 or CN7 pin 24
   - ESV on the ST morpho connector CN7 Pin 6 (5V, O.5A)
   - VBUS_STLK from a USB charger via the ST-LINK USB connector
   - VBUSC from the USB user connector (5V, 0.5A)
   - 3V3_EXT supplied via a pin header CN6 pin 4 or CN7 pin 16 (3.3V, 1.3A)

- On-board ST-LINK/V3EC debugger/programmer

  - mass storage
  - Virtual COM port
  - debug port

- One user LED shared with ARDUINO |reg| Uno V3
- Two push-buttons: USER and RESET
- 32.768 kHz crystal oscillator

More information about the board can be found at the `NUCLEO_H533RE website`_.

Hardware
********

The STM32H533xx devices are high-performance microcontrollers from the STM32H5
Series based on the high-performance Arm |reg| Cortex |reg|-M33 32-bit RISC core.
They operate at a frequency of up to 250 MHz.

- Core: ARM |reg| 32-bit Cortex |reg| -M33 CPU with TrustZone |reg| and FPU.
- Performance benchmark:

  - 375 DMPIS/MHz (Dhrystone 2.1)

- Security

  - Arm |reg| TrustZone |reg| with Armv8-M mainline security extension
  - Up to eight configurable SAU regions
  - TrustZone |reg| aware and securable peripherals
  - Flexible life cycle scheme with secure debug authentication
  - SESIP3 and PSA Level 3 certified assurance target
  - Preconfigured immutable root of trust (ST-iROT)
  - SFI (secure firmware installation)
  - Root of trust thanks to unique boot entry and secure hide protection area (HDP)
  - Secure data storage with hardware unique key (HUK)
  - Secure firmware upgrade support with TF-M
  - Two AES coprocessors including one with DPA resistance
  - Public key accelerator, DPA resistant
  - On-the-fly decryption of Octo-SPI external memories
  - HASH hardware accelerator
  - True random number generator, NIST SP800-90B compliant
  - 96-bit unique ID
  - Active tampers

- Clock management:

  - 24 MHz crystal oscillator (HSE)
  - 32 kHz crystal oscillator for RTC (LSE)
  - Internal 64 MHz (HSI) trimmable by software
  - Internal low-power 32 kHz RC (LSI)( |plusminus| 5%)
  - Internal 4 MHz oscillator (CSI), trimmable by software
  - Internal 48 MHz (HSI48) with recovery system
  - 3 PLLs for system clock, USB, audio, ADC

- Power management

  - Embedded regulator (LDO) with three configurable range output to supply the digital circuitry
  - Embedded SMPS step-down converter

- RTC with HW calendar, alarms and calibration
- Up to 112 fast I/Os, most 5 V-tolerant, up to 10 I/Os with independent supply down to 1.08 V
- Up to 16 timers and 2 watchdogs

  - 8x 16-bit
  - 2x 32-bit timers with up to 4 IC/OC/PWM or pulse counter and quadrature (incremental) encoder input
  - 2x 16-bit low-power 16-bit timers (available in Stop mode)
  - 2x watchdogs
  - 2x SysTick timer

- Memories

  - Up to 512 Kbytes Flash, 2 banks read-while-write
  - 1 Kbyte OTP (one-time programmable)
  - 272 Kbytes of SRAM (80-Kbyte SRAM2 with ECC)
  - 2 Kbytes of backup SRAM available in the lowest power modes
  - Flexible external memory controller with up to 16-bit data bus: SRAM, PSRAM, FRAM, NOR/NAND memories
  - 1x OCTOSPI memory interface with on-the-fly decryption and support for serial PSRAM/NAND/NOR, Hyper RAM/Flash frame formats
  - 1x SD/SDIO/MMC interfaces

- Rich analog peripherals (independent supply)

  - 2x 12-bit ADC with up to 5 MSPS in 12-bit
  - 1x 12-bit DAC with 2 channels
  - 1x Digital temperature sensor
  - Voltage reference buffer

- 34x communication interfaces

  - 1x USB Type-C / USB power-delivery controller
  - 1x USB 2.0 full-speed host and device (crystal-less)
  - 3x I2C FM+ interfaces (SMBus/PMBus)
  - 2x I3C interface
  - 6x U(S)ARTS (ISO7816 interface, LIN, IrDA, modem control)
  - 1x LP UART
  - 4x SPIs including 3 muxed with full-duplex I2S
  - 4x additional SPI from 4x USART when configured in Synchronous mode
  - 2x FDCAN
  - 1x SDMMC interface
  - 2x 16 channel DMA controllers
  - 1x 8- to 14- bit camera interface
  - 1x HDMI-CEC
  - 1x 16-bit parallel slave synchronous-interface

- Development support: serial wire debug (SWD), JTAG, Embedded Trace Macrocell |trade|

More information about STM32H533RE can be found here:

- `STM32H533re on www.st.com`_
- `STM32H533 reference manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Zephyr board options
====================

The STM32H533 is a SoC with Cortex-M33 architecture. Zephyr provides support
for building for Secure firmware.

The BOARD options are summarized below:

+----------------------+-----------------------------------------------+
|   BOARD              | Description                                   |
+======================+===============================================+
| nucleo_h533re        | For building Secure firmware                  |
+----------------------+-----------------------------------------------+

Connections and IOs
===================

Nucleo H533RE Board has 8 GPIO controllers. These controllers are responsible for pin muxing,
input/output, pull-up, etc.

For more details please refer to `STM32H5 Nucleo-64 board User Manual`_.

Default Zephyr Peripheral Mapping:
----------------------------------

- ADC1 channel 0 input: PA0
- USART1 TX/RX : PB14/PB15 (Arduino USART1)
- SPI1 SCK/MISO/MOSI/NSS: PA5/PA6/PA7/PC9
- UART2 TX/RX : PA2/PA3 (VCP)
- USER_PB : PC13

System Clock
------------

Nucleo H533RE System Clock could be driven by internal or external oscillator,
as well as main PLL clock. By default System clock is driven by PLL clock at
240MHz, driven by an 24MHz high-speed external clock.

Serial Port
-----------

Nucleo H533RE board has up to 4 USARTs, 2 UARTs, and one LPUART. The Zephyr console output is assigned
to USART2. Default settings are 115200 8N1.

Backup SRAM
-----------

In order to test backup SRAM, you may want to disconnect VBAT from VDD_MCU.
You can do it by removing ``SB38`` jumper on the back side of the board.
VBAT can be provided via the left ST Morpho connector's pin 33.

Programming and Debugging
*************************

Nucleo H533RE board includes an ST-LINK/V3EC embedded debug tool interface.
This probe allows to flash the board using various tools.

Applications for the ``nucleo_h533re`` board can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

OpenOCD Support
===============

For now, openocd support  for stm32h5 is not available on upstream OpenOCD.
You can check `OpenOCD official Github mirror`_.
In order to use it though, you should clone from the customized
`STMicroelectronics OpenOCD Github`_ and compile it following usual README guidelines.
Once it is done, you can set the OPENOCD and OPENOCD_DEFAULT_PATH variables in
:zephyr_file:`boards/st/nucleo_h533re/board.cmake` to point the build
to the paths of the OpenOCD binary and its scripts,  before
including the common openocd.board.cmake file:

   .. code-block:: none

      set(OPENOCD "<path_to_openocd_repo>/src/openocd" CACHE FILEPATH "" FORCE)
      set(OPENOCD_DEFAULT_PATH <path_to_opneocd_repo>/tcl)
      include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

Flashing
========

The board is configured to be flashed using west `STM32CubeProgrammer`_ runner,
so its :ref:`installation <stm32cubeprog-flash-host-tools>` is required.

Alternatively, OpenOCD, JLink, or pyOCD can also be used to flash the board using
the ``--runner`` (or ``-r``) option:

.. code-block:: console

   $ west flash --runner openocd
   $ west flash --runner pyocd
   $ west flash --runner jlink

For pyOCD, additional target information needs to be installed
which can be done by executing the following commands:

.. code-block:: console

   $ pyocd pack --update
   $ pyocd pack --install stm32h5

Flashing an application to Nucleo H533RE
----------------------------------------

Connect the Nucleo H533RE to your host computer using the USB port.
Then build and flash an application. Here is an example for the
:zephyr:code-sample:`hello_world` application.

Run a serial host program to connect with your Nucleo board:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

Then build and flash the application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucleo_h533re
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   Hello World! nucleo_h533re

Debugging
=========

You can debug an application in the usual way. Here is an example for the
:zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: nucleo_h533re
   :goals: debug

.. _NUCLEO_H533RE website:
   https://www.st.com/en/evaluation-tools/nucleo-h533re

.. _STM32H5 Nucleo-64 board User Manual:
   https://www.st.com/resource/en/user_manual/um3121-stm32h5-nucleo64-board-mb1814-stmicroelectronics.pdf

.. _STM32H533RE on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32h533re

.. _STM32H533 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0481-stm32h533-stm32h563-stm32h573-and-stm32h562-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html

.. _OpenOCD official Github mirror:
   https://github.com/openocd-org/openocd/

.. _STMicroelectronics OpenOCD Github:
   https://github.com/STMicroelectronics/OpenOCD/tree/openocd-cubeide-r6
