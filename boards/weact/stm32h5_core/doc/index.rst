.. zephyr:board:: weact_stm32h5_core

Overview
********

The ``weact_stm32h5_core`` board is a compact development board equipped with
an STM32H562RGT6 microcontroller. It features basic set of peripherals:
user LED and button, microSD |trade| card slot, and combined SWD & UART header.

Key Features

- STM32 microcontroller in LQFP64 package
- USB OTG or full-speed device
- 1 user LED
- User, boot, and reset push-buttons
- 32.768 kHz and 8MHz HSE crystal oscillators
- Board connectors:

   - microSD |trade| card
   - USB Type-C Connector
   - SWD & UART header for external debugger
   - 2x 30-pin GPIO connector

More information about the board can be found on the `WeAct GitHub`_.

Hardware
********

The ``weact_stm32h5_core`` board provides the following hardware components:

   - STM32H562RGT6 in LQFP64 package
   - ARM 32-bit Cortex-M33 CPU with FPU
   - CORDIC for trigonometric functions acceleration
   - FMAC (filter mathematical accelerator)
   - CRC calculation unit
   - 240 MHz max CPU frequency
   - VDD from 1.71 V to 3.6 V
   - 1MB Flash, 2 banks read-while-write
   - 640kB SRAM
   - 4 Kbytes of backup SRAM available in the lowest power modes
   - 2x watchdogs
   - 2x SysTick timer
   - 32-bit timers (2)
   - 16-bit advanced motor control timers (2)
   - 16-bit low power timers (6)
   - 16-bit timers (10)
   - 1x USB Type-C / USB power-delivery controller
   - 1x USB 2.0 full-speed host and device
   - 4x I2C FM+ interfaces (SMBus/PMBus)
   - 1x I3C interface
   - 12x U(S)ARTS (ISO7816 interface, LIN, IrDA, modem control)
   - 1x LP UART
   - 6x SPIs including 3 muxed with full-duplex I2S
   - 2x SAI
   - 1x FDCAN
   - Flexible external memory controller with up to 16-bit data bus: SRAM, PSRAM, FRAM, SDRAM/LPSDR SDRAM, NOR/NAND memories
   - 1x OCTOSPI memory interface with on-the-fly decryption and support for serial PSRAM/NAND/NOR, Hyper RAM/Flash frame formats
   - 1x SD/SDIO/MMC interfaces
   - 1x HDMI-CEC
   - 2x 12-bit ADC with up to 5 MSPS in 12-bit
   - 1x 12-bit D/A with 2 channels
   - 1x Digital temperature sensor

More information about STM32H562RG can be found here:

- `STM32H562RG on www.st.com`_
- `STM32H562 reference manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Pin Mapping
===========

Default Zephyr Peripheral Mapping:
----------------------------------

The ``weact_stm32h5_core`` board is configured as follows

- USER_LED : PB2
- USER_PB : PC13
- SDMMC1 CLK/DCMD/CD/D0/D1/D2/D3 : PC12/PD2/PD4/PC8/PC9/PC10/PC11 (microSD card)
- USB DM/DP : PA11/PA12 (USB CDC ACM)
- UART on debug header : RX/TX - pA10/PA9

System Clock
============

The STM32H562RG System Clock can be driven by an internal or external oscillator,
as well as by the main PLL clock. By default, the System clock is driven
by the PLL clock at 240MHz. PLL clock is fed by a 8MHz external clock.

Serial Port (USB CDC ACM)
=========================

The Zephyr console output is assigned to the USB CDC ACM virtual serial port.
Virtual COM port interface. Default communication settings are 115200 8N1.

Programming and Debugging
*************************

The ``weact_stm32h5_core`` board facilitates firmware flashing via the USB DFU
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

Flashing an application to ``weact_stm32h5_core``
-------------------------------------------------

Here is an example for the :zephyr:code-sample:`hello_world` application.

First, put the board in bootloader mode as described above. Then build and flash
the application in the usual way. Just add ``CONFIG_BOOT_DELAY=5000`` to the
configuration, so that USB CDC ACM is initialized before any text is printed,
as below:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: weact_stm32h5_core
   :goals: build flash
   :gen-args: -DCONFIG_BOOT_DELAY=5000

Run a serial host program to connect with your board:

.. code-block:: console

   $ minicom -D <tty_device> -b 115200

Then, press the RESET button, you should see the following message after few seconds:

.. code-block:: console

   Hello World! weact_stm32h5_core

Replace :code:`<tty_device>` with the port where the board can be found.
For example, under Linux, :code:`/dev/ttyACM0`.

Debugging
---------

This current Zephyr port does not support debugging.

Testing the LEDs in the ``weact_stm32h5_core``
**********************************************

There is a sample that allows to test that LED on the board are working
properly with Zephyr:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: weact_stm32h5_core
   :goals: build flash
   :gen-args: -DCONFIG_BOOT_DELAY=5000

You can build and flash the examples to make sure Zephyr is running correctly on
your board. The LED definitions can be found in
:zephyr_file:`boards/weact/stm32h5_core/weact_stm32h5_core.dts`.

Testing shell over USB in the ``weact_stm32h5_core``
****************************************************

There is a sample that allows to test shell interface over USB CDC ACM interface
with Zephyr:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/shell/shell_module
   :board: weact_stm32h5_core
   :goals: build flash
   :gen-args: -DCONFIG_BOOT_DELAY=5000

.. _WeAct GitHub:
   https://github.com/WeActStudio/WeActStudio.STM32H5_64Pin_CoreBoard

.. _STM32H562RG on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32h562rg.html

.. _STM32H562 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0481-stm32h52333xx-stm32h56263xx-and-stm32h573xx-armbased-32bit-mcus-stmicroelectronics.pdf
