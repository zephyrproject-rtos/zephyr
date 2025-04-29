.. zephyr:board:: stm32f429ii_aca

Overview
********

The IAR STM32F429II-ACA evaluation board features an ARM Cortex-M4 based STM32F429II MCU.
Here are some highlights of the STM32F429II-ACA board:

- STM32 microcontroller in LQFP144 package
- JTAG/SWD debugger/programmer interface
- Flexible board power supply

  - JTAG/SWD connector
  - USB HS connector

- 3x user push-buttons and 1x RESET push-button
- Open-close switch and on-auto-off switch
- 2x capacitive touch panels
- USB OTG with mini-USB connector
- Small speaker
- Trimmer potentiometer
- Nine LEDs

  - 1x power LED
  - 3x car traffic light LEDs
  - 2x pedestrian traffic light LEDs
  - 1x car interior light LED
  - 2x user LEDs

Schematics for the board can be found `here <stm32f429ii-aca-schematics_>`_

Hardware
********

The STM32F429II-ACA evaluation board provides the following hardware components:

- STM32F429II in LQFP144 package
- ARM |reg| 32-bit Cortex |reg| -M4 CPU with FPU
- 180 MHz max CPU frequency
- VDD from 1.8 V to 3.6 V
- 2 MB Internal Flash
- 4 Mbit External Flash
- 256+4 KB SRAM including 64-KB of core coupled memory
- GPIO with external interrupt capability
- 12-bit ADC
- 12-bit DAC
- RTC
- General Purpose Timers
- I2C
- SPI
- USB 2.0 OTG HS/FS with dedicated DMA, on-chip full-speed PHY and ULPI
- CRC calculation unit
- True random number generator
- DMA Controller

More information about STM32F429II can be found here:

- `STM32F429II on www.st.com`_
- `STM32F429 Reference Manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Default Zephyr Peripheral Mapping:
----------------------------------
- I2C_1_SCL : PB8
- I2C_1_SDA : PB7
- I2C_2_SCL : PH4
- I2C_2_SDA : PH5
- SPI_5_NSS : PF6
- SPI_5_SCK : PF7
- SPI_5_MISO : PF8
- SPI_5_MOSI : PF9
- OTG_HS_ID : PB12
- OTG_HS_DM : PB14
- OTG_HS_DP : PB15

Serial Port
===========

By default, the STM32F429II-ACA evaluation board has no physical serial port available.
The board has up to 8 UARTs, of which none are used.

USB Port
========

The STM32F429II-ACA evaluation board has a USB HS capable Mini-USB port. It is connected to the on-chip
OTG_HS peripheral.

Programming and Debugging
*************************

Applications for the ``stm32f429ii_aca`` board configuration can be built
and flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

In order to flash this board using west, an external debug probe such as a Segger J-Link
has to be connected through the JTAG/SWD connector on the board.
By default, the board is set to be flashed using the jlink runner.
Alternatively, openocd, or pyocd can also be used as runners to flash the board using
the ``--runner`` (or ``-r``) option:

.. code-block:: console

   $ west flash --runner openocd
   $ west flash --runner pyocd

First, connect the STM32F429II-ACA evaluation board to your host computer using
your debug probe through the JTAG/SWD connector to prepare it for flashing.
Then build and flash your application.

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: stm32f429ii_aca
   :goals: build flash

LED0 should then begin to blink continuously with a 1-second delay.

References
**********

.. target-notes::

.. _stm32f429ii-aca-schematics:
   https://iar.my.salesforce.com/sfc/p/#30000000YATY/a/Qx000000vZVh/EzlIqYKIBVXN8PN4Q8MgtowSZrR_vZarwLiNJXw7UJw

.. _STM32F429II on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32f429ii.html

.. _STM32F429 Reference Manual:
   https://www.st.com/content/ccc/resource/technical/document/reference_manual/3d/6d/5a/66/b4/99/40/d4/DM00031020.pdf/files/DM00031020.pdf/jcr:content/translations/en.DM00031020.pdf
