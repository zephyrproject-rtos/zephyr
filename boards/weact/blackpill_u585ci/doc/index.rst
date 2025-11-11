.. zephyr:board:: blackpill_u585ci

Overview
********

The WeAct Black Pill STM32U585CI Core Board is an extremely low cost and bare-bone
development board featuring the STM32U585CI, see `STM32U585CI website`_.
This is the 48-pin variant of the STM32U585C series,
see `STM32U5 reference manual`_. More info about the board available
on `WeAct Github`_.

Key Features

- STM32U585CI microcontroller in UFQFPN48 package
- ARM |reg| 32-bit Cortex |reg| -M33 with TrustZone |reg|, MPU, DSP and FPU
- 160 MHz max CPU frequency, 1.5 DMPIS/MHz, 4.07 CoreMark |reg| /MHz
- 2 MB Flash with ECC and 2 banks
- 784 KB SRAM
- GPIO with external interrupt capability
- 1x14-bit 2.5 MSPS ADC, 1x12-bit 2.5 MSPS ADC
- 2x12-bit DAC
- USB OTG 2.0 full-speed controller
- 1 user LED
- User, boot, and reset push-buttons
- 32.768 kHz and 8MHz HSE crystal oscillators
- Board connectors:

   - USB Type-C |reg| Connector
   - SWD header for external debugger
   - 2x 20-pin GPIO connector

Hardware
********

The STM32U585xx devices are an ultra-low-power microcontrollers family (STM32U5
Series) based on the high-performance Arm |reg| Cortex |reg| -M33 32-bit RISC core.
They operate at a frequency of up to 160 MHz.
More information about STM32U585CI can be found here:

- `STM32U585CI website`_
- `STM32U5 reference manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Pin Mapping
===========

Default Zephyr Peripheral Mapping:
----------------------------------

- USER_LED : PC13
- USER_PB : PA0
- USB DM/DP : PA11/PA12 (USB CDC ACM)
- UART : RX/TX - PA10/PA9
- LPUART : RX/TX - PA3/PA2
- I2C1 : SCL/SDA - PB6/PB3
- I2C2 : SCL/SDA - PB10/PB14
- SPI1 : SCK/MISO/MOSI/NSS - PA1/PA6/PA7/PA4
- FDCAN : RX/TX - PB8/PB9

System Clock
============

The STM32U585CI System Clock could be driven by internal or external oscillator,
as well as main PLL clock. By default System clock is driven by PLL clock at
160MHz, driven by 25MHz external clock.

Serial Port (USB CDC ACM)
=========================

The Zephyr console output is assigned to the USB CDC ACM virtual serial port.
Virtual COM port interface. Default communication settings are 115200 8N1.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The ``blackpill_u585ci`` board facilitates firmware flashing via the USB DFU
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


Flashing an application to ``blackpill_u585ci``
-----------------------------------------------

Here is an example for the :zephyr:code-sample:`hello_world` application.

First, put the board in bootloader mode as described above. Then build and flash
the application in the usual way. Just add ``CONFIG_BOOT_DELAY=5000`` to the
configuration, so that USB CDC ACM is initialized before any text is printed,
as below:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/console
   :board: blackpill_u585ci
   :goals: build flash
   :gen-args: -DCONFIG_BOOT_DELAY=5000

Run a serial host program to connect with your board:

.. code-block:: console

   $ minicom -D <tty_device> -b 115200

Then, press the RESET button, you should see the following message after few seconds:

.. code-block:: console

   Hello World! arm

Replace :code:`<tty_device>` with the port where the board can be found.
For example, under Linux, :code:`/dev/ttyACM0`.

Debugging
---------

There is support for `Black Magic Probe`_ debugger.
At minimum connect GND, SWDIO and SWCLK lines.
Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: blackpill_u585ci
   :goals: debug

References
**********

.. target-notes::

.. _Zadig:
   https://zadig.akeo.ie/

.. _WeAct Github:
   https://github.com/WeActStudio/WeActStudio.STM32U585Cx_CoreBoard

.. _dfu-util:
   http://dfu-util.sourceforge.net/build.html

.. _STM32U585CI website:
   https://www.st.com/en/microcontrollers-microprocessors/stm32u585ci.html

.. _STM32U5 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0456-stm32u5-series-armbased-32bit-mcus-stmicroelectronics.pdf

.. _Black Magic Probe:
   https://black-magic.org/index.html
