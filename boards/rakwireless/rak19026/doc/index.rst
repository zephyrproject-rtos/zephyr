.. zephyr:board:: rak19026

Overview
********

RAK19026 VC is a WisBlock Base Board that connects WisBlock IO, and
WisBlock Modules. It provides the power supply and interconnection to
the modules attached to it. Different to other WisBlock Base Boards,
it has no Core Slot for a WisBlock Core module. Instead it has a WisDuo
RAK4630 integrated as its MCU. Beside of the MCU, a GNSS module and
acceleration sensor are as well on the Base Board. Similar to other
WisBlock Base Boards it has two slots C-D for WisBlock modules.
The WisBlock modules are attached to the top side of the RAK19026 VC.
The Slot C and D support 10 mm WisBlock modules. In addtion it has
one IO slot for WisBlock IO modules.

For convenience, there is a USB-C connector that is connected directly
to WisDuo MCU’s USB port. It can be used for uploading firmware or
serial communication. The USB-C connector is also used as a battery
charging port.

WisBlock modules are connected to the RAK19026 VC WisBlock Base board
via high-speed board-to-board connectors. They provide secure and
reliable interconnection to ensure the signal integrity of each data bus.
A set of screws are used for fixing the modules, which makes it reliable
even in an environment with lots of vibrations.

Hardware
********

Supported Features
==================

- 1 WisBlock modules compatible with IO slot
- 2 WisBlock modules compatible with slots C-D
- 1 Type-C USB port for programming and debugging
- 3.7 V Rechargeable battery connector
   - 3 pin connector for EU/UK markets (CE & UKCA certifications)
   - 2 pin connector for other markets
- 5 V Solar panel connector
- Multiple headers with solder contacts
   - 4 pin header with SCL, SDA, AIN1, and IO8
   - 4 pin header with IO3, IO4, IO6, and IO7
   - 4 pin header with 3V3_S, GND, 3.3V, and GND
   - 4 pin header for OLED display with I2C, 3.3V, and GND
   - 4 pin header with SWD, SWCLK, RST, and GND (for programming and debugging with RAKDAP1 or Jlink adapters)

.. zephyr:board-supported-hw::

Programming and debugging
*************************

.. zephyr:board-supported-runners::

Building & Flashing
===================

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: rak19026/nrf52840
   :goals: build flash

Debugging
=========

You can debug an application in the usual way. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rak19026/nrf52840
   :maybe-skip-config:
   :goals: debug

References
**********
