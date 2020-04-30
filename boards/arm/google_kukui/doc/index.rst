.. _google_kukui_board:

Google Kukui EC
###############

Overview
********

Kukui is a reference board for Chromium OS-based devices Krane and
Kodama. These are known as the Lenovo Chromebook Duet and 10e Chromebook
Tablet, respectively.

Zephyr has support for the STM32-based embedded controller (EC) on-board.

Hardware
********

- STM32F098RCH6
- MT6370 battery charger
- BMM150 compass
- BMM160 gyroscope
- Connections to the MediaTek AP

Supported Features
==================

The following features are supported:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| CLOCK     | on-chip    | reset and clock control             |
+-----------+------------+-------------------------------------+
| FLASH     | on-chip    | flash memory                        |
+-----------+------------+-------------------------------------+
| WATCHDOG  | on-chip    | independent watchdog                |
+-----------+------------+-------------------------------------+

Other features (such as I2C) are not available in Zephyr.

The default configuration can be found in the defconfig file:
``boards/arm/google_kukui/google_kukui_defconfig``

Connections and IOs
===================

Each of the GPIO pins can be configured by software as output
(push-pull or open-drain), as input (with or without pull-up or
pull-down), or as peripheral alternate function.

Default Zephyr Peripheral Mapping:
----------------------------------

- UART_1 TX/RX : PA10/PA9
- I2C_1 SCL/SDA : PB8/PB9
- I2C_2 SCL/SDA : PA11/PA12
- Volume down : GPIOB pin 11
- Volume up : GPIOB pin 10
- Power : GPIOA pin 0

Programming and Debugging
*************************

Build application as usual for the ``google_kukui`` board, and flash
using Servo V2, μServo, or Servo V4 (CCD). See the
`Chromium EC Flashing Documentation`_ for more information.

Debugging
=========

Use SWD with a J-Link or ST-Link.

References
**********

.. target-notes::

.. _Chromium EC Flashing Documentation:
   https://chromium.googlesource.com/chromiumos/platform/ec#Flashing-via-the-servo-debug-board
