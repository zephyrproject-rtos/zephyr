.. mini_stm32h743:

WeAct Studio MiniSTM32H743 Core Board
#####################################

Overview
********

The MiniSTM32H743 Core board is a compact development board equipped with
an STM32H743VIT6 microcontroller. It features a variety of peripherals,
including a user LED and button, a display, and external SPI and QuadSPI
NOR flash memory.

Key Features

- STM32 microcontroller in LQFP100 package
- USB OTG or full-speed device
- 1 user LED
- User, boot, and reset push-buttons
- 32.768 kHz and 25MHz HSE crystal oscillators
- External NOR Flash memories: 64-Mbit Quad-SPI and 64-Mbit SPI
- Board connectors:
   - Camera (8 bit) connector
   - ST7735 TFT-LCD 160 x 80 pixels (RGB565 3-SPI)
   - microSD |trade| card
   - USB Type-C Connector
   - SWD header for external debugger
   - 2x 40-pin GPIO connector

.. figure:: img/stm32h7xx.webp
      :align: center
      :alt: MiniSTM32H743 Core Board

      MiniSTM32H743 Core Board (Credit: WeAct Studio)

More information about the board can be found on the `Mini_STM32H743 website`_.

Hardware
********

The MiniSTM32H743 Core board provides the following hardware components:

- STM32H743VIT6 in LQFP100 package
- ARM 32-bit Cortex-M7 CPU with FPU
- Chrom-ART Accelerator
- Hardware JPEG Codec
- 480 MHz max CPU frequency
- VDD from 1.62 V to 3.6 V
- 2 MB Flash
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

More information about STM32H743 can be found here:

- `STM32H743VI on www.st.com`_
- `STM32H743/753 reference manual`_
- `STM32H743VI datasheet`_

Supported Features
==================

The mini_stm32h743 board configuration supports the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| WDT       | on-chip    | watchdog                            |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| FLASH     | on-chip    | flash memory                        |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| SPI       | on-chip    | spi                                 |
+-----------+------------+-------------------------------------+
| QSPI NOR  | on-chip    | off-chip flash                      |
+-----------+------------+-------------------------------------+
| SDMMC     | on-chip    | disk access                         |
+-----------+------------+-------------------------------------+
| DISPLAY   | on-chip    | display                             |
+-----------+------------+-------------------------------------+

Other hardware features have not been enabled yet for this board.

The default configuration per core can be found in the defconfig file:
``boards/weact/mini_stm32h743/mini_stm32h743_defconfig``

Pin Mapping
===========

MiniSTM32H743 Core board has 5 GPIO controllers. These controllers are responsible for pin muxing,
input/output, pull-up, etc.

For more details please refer to `Mini_STM32H743 website`_.

Default Zephyr Peripheral Mapping:
----------------------------------

The MiniSTM32H743 Core board is configured as follows

- USER_LED : PE3
- USER_PB : PC13
- SPI1 SCK/MISO/MOSI/NSS : PB3/PB4/PD7/PD6 (NOR Flash memory)
- SPI4 SCK/MOSI/NSS : PE12/PE14/PE11 (LCD)
- QuadSPI CLK/NCS/IO0/IO1/IO2/IO3 : PB2/PB6/PD11/PD12/PE2/PD13 (NOR Flash memory)
- SDMMC1 CLK/DCMD/CD/D0/D1/D2/D3 : PC12/PD2/PD4/PC8/PC9/PC10/PC11 (microSD card)
- USB DM/DP : PA11/PA12 (USB CDC ACM)

System Clock
============

The STM32H743VI System Clock can be driven by an internal or external oscillator,
as well as by the main PLL clock. By default, the System clock is driven
by the PLL clock at 240MHz. PLL clock is fed by a 25MHz high speed external clock.

Serial Port (USB CDC ACM)
=========================

The Zephyr console output is assigned to the USB CDC ACM virtual serial port.
Virtual COM port interface. Default communication settings are 115200 8N1.

Programming and Debugging
*************************

The MiniSTM32H743 Core board facilitates firmware flashing via the USB DFU
bootloader. This method simplifies the process of updating images, although
it doesn't provide debugging capabilities. However, the board provides header
pins for the Serial Wire Debug (SWD) interface, which can be used to connect
an external debugger, such as ST-Link.

Flashing
========

To activate the bootloader, follow these steps:

1. Press and hold the BOOT0 key.
2. While still holding the BOOT0 key, press and release the RESET key.
3. Wait for 0.5 seconds, then release the BOOT0 key.

Upon successful execution of these steps, the device will transition into
bootloader mode and present itself as a USB DFU Mode device. You can program
the device using the west tool or the STM32CubeProgrammer.

Flashing an application to MiniSTM32H743
----------------------------------------

Here is an example for the :ref:`hello_world` application.

First, put the board in bootloader mode as described above. Then build and flash
the application in the usual way. Just add ``CONFIG_BOOT_DELAY=5000`` to the
configuration, so that USB CDC ACM is initialized before any text is printed,
as below:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mini_stm32h743
   :goals: build flash
   :gen-args: -DCONFIG_BOOT_DELAY=5000

Run a serial host program to connect with your board:

.. code-block:: console

   $ minicom -D <tty_device> -b 115200

Then, press the RESET button, you should see the following message after few seconds:

.. code-block:: console

   Hello World! mini_stm32h743

Replace :code:`<tty_device>` with the port where the board XIAO BLE
can be found. For example, under Linux, :code:`/dev/ttyACM0`.

Debugging
---------

This current Zephyr port does not support debugging.

Testing the LEDs in the MiniSTM32H743
*************************************

There is a sample that allows to test that LED on the board are working
properly with Zephyr:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: mini_stm32h743
   :goals: build flash
   :gen-args: -DCONFIG_BOOT_DELAY=5000

You can build and flash the examples to make sure Zephyr is running correctly on
your board. The LED definitions can be found in
:zephyr_file:`boards/weact/mini_stm32h743/mini_stm32h743.dts`.

Testing shell over USB in the MiniSTM32H743
*******************************************

There is a sample that allows to test shell interface over USB CDC ACM interface
with Zephyr:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/shell/shell_module
   :board: mini_stm32h743
   :goals: build flash
   :gen-args: -DCONFIG_BOOT_DELAY=5000

.. _Mini_STM32H743 website:
   https://github.com/WeActStudio/MiniSTM32H7xx

.. _STM32H743VI on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32h743vi.html#overview

.. _STM32H743/753 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0433-stm32h742-stm32h743753-and-stm32h750-value-line-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32H743VI datasheet:
   https://www.st.com/resource/en/datasheet/stm32h743vi.pdf
