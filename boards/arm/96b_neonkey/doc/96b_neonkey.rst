.. _96b_neonkey:

96Boards Neonkey
################

Overview
********

96Boards Neonkey board is based on the STMicroelectronics STM32F411CE
Cortex M4 CPU. Zephyr applications use the 96b_neonkey configuration
to run on these boards.

.. figure:: img/96b-neonkey-front.jpg
     :width: 500px
     :align: center
     :height: 330px
     :alt: 96Boards Neonkey

     96Boards Neonkey

This board acts as a sensor hub platform for all 96Boards compliant
family products. It can also be used as a standalone board.

Hardware
********

96Boards Neonkey provides the following hardware components:

- STM32F411CE in UFQFPN48 package
- ARM |reg| 32-bit Cortex |reg|-M4 CPU with FPU
- 84 MHz max CPU frequency
- 1.8V work voltage
- 512 KB Flash
- 128 KB SRAM
- On board sensors:

  - Temperature/Humidity: SI7034-A10
  - Pressure: BMP280
  - ALS/Proximity: RPR-0521RS
  - Geomagnetic: BMM150
  - Acclerometer/Gyroscope: BMI160
  - AMR Hall sensor: MRMS501A
  - Microphone: SPK0415HM4H-B

- 4 User LEDs
- 15 General purpose LEDs
- GPIO with external interrupt capability
- I2C (3)
- SPI (1)
- I2S (1)

Supported Features
==================

The Zephyr 96b_neonkey board configuration supports the following hardware
features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | system clock                        |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port                         |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| FLASH     | on-chip    | flash                               |
+-----------+------------+-------------------------------------+
| SPI       | on-chip    | spi                                 |
+-----------+------------+-------------------------------------+
| I2C       | on-chip    | i2c                                 |
+-----------+------------+-------------------------------------+

More details about the board can be found at `96Boards website`_.

The default board configuration can be found in the defconfig file:

        ``boards/arm/96b_neonkey/96b_neonkey_defconfig``

Connections and IOs
===================

LED
---

- LED1 / User1 LED = PB12
- LED2 / User2 LED = PB13
- LED3 / User3 LED = PB14
- LED4 / User4 LED = PB15

Push buttons
------------

- BUTTON = RST (SW1)
- BUTTON = USR (SW2)

System Clock
============

96Boards Neonkey can be driven by an internal oscillator as well as the main
PLL clock. By default System clock is sourced by PLL clock at 84MHz, driven
by internal oscillator.

Serial Port
===========

On 96Boards Neonkey Zephyr console output is assigned to USART1.
Default settings are 115200 8N1.

I2C
---

96Boards Neonkey board has up to 3 I2Cs. The default I2C mapping for Zephyr is:

- I2C1_SCL : PB6
- I2C1_SDA : PB7
- I2C2_SCL : PB10
- I2C2_SDA : PB3
- I2C3_SCL : PA8
- I2C3_SCL : PB4

Programming and Debugging
*************************

Building
========

Here is an example for building the :ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: 96b_neonkey
   :goals: build flash

Flashing
========

96Boards Neonkey can be flashed by two methods, one using the ROM
bootloader and another using the SWD debug port (which requires additional
hardware).

Using ROM bootloader:
---------------------

ROM bootloader can be triggered by the following pattern:

1. Connect BOOT0 to VDD (link JTAG pins 1 and 5 on P4 header)
2. Press and hold the USR button
3. Press and release the RST button

More detailed information on activating the ROM bootloader can be found in
Chapter 29 of Application note `AN2606`_. The ROM bootloader supports flashing
via UART, I2C and SPI protocols.

For flashing, `stm32flash`_ command line utility can be used. The following
command will flash the ``zephyr.bin`` binary to the Neonkey board using UART
and starts its execution:

.. code-block:: console

   $ stm32flash -w zephyr.bin -v -g 0x08000000 /dev/ttyS0

.. note::
   The above command assumes that Neonkey board is connected to
   serial port ``/dev/ttyS0``.

Using SWD debugger:
-------------------

For flashing via SWD debug port, 0.1" male header must be soldered at P4
header available at the bottom of the board, near RST button.

Use the `Black Magic Debug Probe`_ as an SWD programmer, which can
be connected to the P4 header using its flying leads and its 20 Pin
JTAG Adapter Board Kit. When plugged into your host PC, the Black
Magic Debug Probe enumerates as a USB serial device as documented on
its `Getting started page`_.

It also uses the GDB binary provided with the Zephyr SDK,
``arm-zephyr-eabi-gdb``. Other GDB binaries, such as the GDB from GCC
ARM Embedded, can be used as well.

.. code-block:: console

   $ arm-zephyr-eabi-gdb -q zephyr.elf
   (gdb) target extended-remote /dev/ttyACM0
   Remote debugging using /dev/ttyACM0
   (gdb) monitor swdp_scan
   Target voltage: 1.8V
   Available Targets:
   No. Att Driver
    1      STM32F4xx
   (gdb) attach 1
   Attaching to Remote target
   0x080005d0 in ?? ()
   (gdb) load

Debugging
=========

After flashing 96Boards Neonkey, it can be debugged using the same
GDB instance. To reattach, just follow the same steps above, till
"attach 1". You can then debug as usual with GDB. In particular, type
"run" at the GDB prompt to restart the program you've flashed.

References
**********

.. _96Boards website:
   https://www.96boards.org/product/neonkey/

.. _AN2606:
   https://www.st.com/resource/en/application_note/cd00167594.pdf

.. _stm32flash:
   https://sourceforge.net/p/stm32flash/wiki/Home/

.. _Black Magic Debug Probe:
   https://github.com/blacksphere/blackmagic/wiki

.. _Getting started page:
   https://github.com/blacksphere/blackmagic/wiki/Getting-Started
