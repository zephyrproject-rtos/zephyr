.. zephyr:board:: mini_stm32h750

Overview
********

The MiniSTM32H750 Core board is a compact development board equipped with
an STM32H750VBT6 microcontroller. It features a variety of peripherals,
including a user LED and button, and external SPI and QuadSPI
NOR flash memory.

Key Features

- STM32 microcontroller in LQFP100 package
- USB OTG or full-speed device
- 1 user LED
- User, boot, and reset push-buttons
- 32.768 kHz and 25MHz HSE crystal oscillators
- External NOR Flash memories: 64-Mbit Quad-SPI and 64-Mbit SPI
- Board connectors

  - Camera (8 bit) connector
  - ST7735 TFT-LCD 160 x 80 pixels (RGB565 3-SPI)
  - microSD card
  - USB Type-C Connector
  - SWD header for external debugger
  - 2x 40-pin GPIO connector

More information about the board can be found on the `Mini_STM32H7xx website`_.

Hardware
********

The MiniSTM32H750 Core board provides the following hardware components:

- STM32H750VBT6 in LQFP100 package
- ARM 32-bit Cortex-M7 CPU with FPU
- Chrom-ART Accelerator
- Hardware JPEG Codec
- 480 MHz max CPU frequency
- VDD from 1.62 V to 3.6 V
- 128 KB Flash
- ~1 MB SRAM
- High-resolution timer (2.1 ns)
- 32-bit timers(2)
- 16-bit timers(10)
- SPI(6)
- I2C(4)
- I2S (3)
- USART(4)
- UART(4)
- USB OTG Full Speed and High Speed(1)
- USB OTG Full Speed(1)
- CAN FD(2)
- SAI(4)
- SPDIF_Rx(4)
- HDMI_CEC(1)
- Dual Mode Quad SPI(1)
- Camera Interface
- GPIO (up to 82) with external interrupt capability
- 16-bit ADC(3) with 16 channels
- 12-bit DAC with 2 channels(2)
- True Random Number Generator (RNG)
- 16-channel DMA
- LCD-TFT Controller with XGA resolution

More information about STM32H750 can be found here:

- `STM32H750VB on www.st.com`_
- `STM32H750 reference manual`_
- `STM32H750VB datasheet`_

Supported Features
==================

.. zephyr:board-supported-hw::

Pin Mapping
===========

MiniSTM32H750 Core board has 5 GPIO controllers. These controllers are responsible for pin muxing,
input/output, pull-up, etc.

For more details please refer to `Mini_STM32H7xx website`_.

Default Zephyr Peripheral Mapping:
----------------------------------

The MiniSTM32H750 Core board is configured as follows

- USART1 TX/RX : PB14/PB15 (Console)
- USER_LED : PE3
- USER_PB : PC13
- SPI1 SCK/MISO/MOSI/NSS : PB3/PB4/PD7/PD6 (NOR Flash memory)
- QuadSPI CLK/NCS/IO0/IO1/IO2/IO3 : PB2/PB6/PD11/PD12/PE2/PD13 (NOR Flash memory)
- SDMMC1 CLK/DCMD/CD/D0/D1/D2/D3 : PC12/PD2/PD4/PC8/PC9/PC10/PC11 (microSD card)
- USB DM/DP : PA11/PA12

System Clock
============

The STM32H750VB System Clock can be driven by an internal or external oscillator,
as well as by the main PLL clock. By default, the System clock is driven
by the PLL clock at 480MHz. PLL clock is fed by a 25MHz high speed external clock.

Serial Port
============

The Zephyr console output is assigned to USART1. Default communication
settings are 115200 8N1.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The MiniSTM32H750 Core board facilitates firmware flashing via the USB DFU
bootloader. This method simplifies the process of updating images, although
it doesn't provide debugging capabilities. However, the board provides header
pins for the Serial Wire Debug (SWD) interface, which can be used to connect
an external debugger, such as ST-Link.

When flashing to the external QSPI Flash using STM32CubeProgrammer or
ST-Link GDB Server (e.g. ``--runner stm32cubeprogrammer`` or
``--runner stlink_gdbserver``), the external flash loader
``STM32H7xx_W25Q128_WeActStudio.stldr`` is required. Download it from
`Mini_STM32H7xx QSPI Flasher`_ and copy it into the ``ExternalLoader``
directory of your STM32CubeProgrammer installation.

Flashing
========

To activate the bootloader, follow these steps:

1. Press and hold the BOOT0 key.
2. While still holding the BOOT0 key, press and release the RESET key.
3. Wait for 0.5 seconds, then release the BOOT0 key.

Upon successful execution of these steps, the device will transition into
bootloader mode and present itself as a USB DFU Mode device. You can program
the device using the west tool or the STM32CubeProgrammer.

Application in SoC Flash
========================

Here is an example for how to build and flash the :zephyr:code-sample:`hello_world` application.

First, put the board in bootloader mode as described above. Then build and flash
the application in the usual way:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mini_stm32h750
   :goals: build flash

Run a serial host program to connect with your board:

.. code-block:: console

   $ minicom -D <tty_device> -b 115200

Then, press the RESET button, you should see the following message:

.. code-block:: console

   Hello World! mini_stm32h750

Replace :code:`<tty_device>` with the port where the board
can be found. For example, under Linux, :code:`/dev/ttyUSB0`.

If the application size is too big to fit in SoC Flash,
Zephyr :ref:`Code and Data Relocation <code_data_relocation>` can be used to relocate
the non-critical and big parts of the application to external Flash.

Debugging
---------

You can debug an application using OpenOCD or J-Link via the SWD header.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mini_stm32h750
   :goals: debug

Application in External Flash
=============================

Because of the limited amount of SoC Flash (128KB), you may want to store the application
in external QSPI Flash instead, and run it from there. In that case, the MCUboot bootloader
is needed to chainload the application. A dedicated board variant, ``ext_flash_app``, was created
for this usecase.

:ref:`sysbuild` makes it possible to build and flash all necessary images needed to run a user application
from external Flash.

The following example shows how to build :zephyr:code-sample:`hello_world` with Sysbuild enabled:

.. zephyr-app-commands::
   :tool: west
   :zephyr-app: samples/hello_world
   :board: mini_stm32h750/stm32h750xx/ext_flash_app
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

   *** Booting MCUboot v2.2.0-173-gb192716c969a ***
   *** Using Zephyr OS build v4.2.0-6260-ge39ba1a35bc4 ***
   I: Starting bootloader
   I: Image index: 0, Swap type: none
   I: Image index: 0, Swap type: none
   I: Bootloader chainload address offset: 0x0
   I: Image version: v0.0.0
   I: Jumping to the first image slot
   *** Booting Zephyr OS build v4.2.0-6260-ge39ba1a35bc4 ***
   Hello World! mini_stm32h750/stm32h750xx/ext_flash_app

To only flash the user application in the subsequent builds, use:

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

Testing the LEDs in the MiniSTM32H750
*************************************

There is a sample that allows to test that LED on the board are working
properly with Zephyr:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: mini_stm32h750
   :goals: build flash

You can build and flash the examples to make sure Zephyr is running correctly on
your board. The LED definitions can be found in
:zephyr_file:`boards/weact/mini_stm32h750/mini_stm32h750-common.dtsi`.

.. _Mini_STM32H7xx website:
   https://github.com/WeActStudio/MiniSTM32H7xx

.. _STM32H750VB on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32h750vb.html#overview

.. _STM32H750 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0433-stm32h742-stm32h743753-and-stm32h750-value-line-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32H750VB datasheet:
   https://www.st.com/resource/en/datasheet/stm32h750vb.pdf

.. _MCUboot design:
   https://docs.mcuboot.com/design.html

.. _Mini_STM32H7xx QSPI Flasher:
   https://github.com/WeActStudio/MiniSTM32H7xx/tree/master/SDK/QSPI_Flasher

.. _MCUboot Encryption:
   https://docs.mcuboot.com/encrypted_images.html
