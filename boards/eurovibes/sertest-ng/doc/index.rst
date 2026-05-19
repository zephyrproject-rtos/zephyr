.. zephyr:board:: eurovibes_stm32g431_sertest-ng

The Eurovibes STM32G431 based Sertest-NG board is a universal serial protocol
test board. It supports one full RS232 port, a two wire RS232 board, a RS422,
a RS485 and a CAN-FD port. LEDs show the different line status information.
See the `STM32G431RB website`_ for more information about the MCU. More
information about the board, including schematics, is available from the
`Eurovibes GitHub`_.

Supported Features
==================

.. zephyr:board-supported-hw::

Pin Mapping
===========

Default Zephyr Peripheral Mapping:
----------------------------------

- FDCAN_1 TX/RX  : PB9/PB8
- I2C2 SDA/SCL   : PA8/PA9
- LED0           : PB4
- LED1           : PB5
- LED2           : PB6
- LED3           : PB7
- USART_1 TX/RX  : PC4/PBC5
- USART_2 TX/RX  : PA2/PA3
- USART_2 CTS/RTS: PA0/PA1
- USART_3 TX/RX  : PB10/PB11

Hardware Configuration
----------------------
+---------------+---------+-----------------------------------------------+
| Solder bridge | Default | Description                                   |
+===============+=========+===============================================+
| JP101         | Open    | M24C64 write protection: close to enable WR   |
+---------------+---------+-----------------------------------------------+
| JP102         | 1-2     | VBAT: 1-2: 1F Capacitor / 2-3: bypass         |
+---------------+---------+-----------------------------------------------+
| JP103         | Open    | BOOT0: close to enter programing mode         |
+---------------+---------+-----------------------------------------------+
| JP401         | Open    | connect CAN GND to GND                        |
+---------------+---------+-----------------------------------------------+
| JP402         | 1-2     | CAN TX: 1-2 default / 2-3 swap                |
+---------------+---------+-----------------------------------------------+
| JP403         | 1-2     | CAN RX: 1-2 default / 2-3 swap                |
+---------------+---------+-----------------------------------------------+


Clock Sources
-------------

The board has two external oscillators. The frequency of the slow clock (LSE)
is 32.768 kHz. The frequency of the main clock (HSE) is 8 MHz.

The default configuration sources the system clock from the PLL, which is
derived from HSE, and is set at 170 MHz. The 48 MHz clock used by the USB
interface is derived from the internal 48 MHz oscillator.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The MCU is normally programmed using the exposed SWD port (Tag-Connect TC2030).

Flashing an Application
=======================

Connect a Tag-Connect cable from a J-Link JTAG probe to the board and the
board should power ON.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: eurovibes_stm32g431_sertest-ng
   :goals: build flash

Debugging
=========

The board can be debugged by connecting a JTAG probe to the Tag-Connect
connector:

Tag-Connect TC2030 SWD pinout
-----------------------------
+-----+-----------------------------------+
| Pin | Signal                            |
+=====+===================================+
| 1   | VCC                               |
+-----+-----------------------------------+
| 2   | SWDIO / TMS                       |
+-----+-----------------------------------+
| 3   | nRESET                            |
+-----+-----------------------------------+
| 4   | SWCLK / TCK                       |
+-----+-----------------------------------+
| 5   | GND (also connected to GNDDetect) |
+-----+-----------------------------------+
| 6   | SWO / TDO                         |
+-----+-----------------------------------+


References
**********

.. target-notes::

.. _Eurovibes GitHub:
   https://github.com/eurovibes/sertest-ng

.. _STM32G431RB website:
   https://www.st.com/en/microcontrollers-microprocessors/stm32g431rb.html

.. _STM32G4 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
