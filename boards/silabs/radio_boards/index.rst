.. _silabs_radio_boards:

Radio Boards
############

.. toctree::
   :maxdepth: 1
   :titlesonly:
   :glob:

   **/*

Overview
********

Radio Boards are used together with a Wireless Mainboard, which is a
development platform for application development and debugging of wireless
products.

There are two main variants of the Mainboard:

- Wireless Starter Kit Mainboard (board BRD4001A, available standalone as SLWMB4001A)
- Wireless Pro Kit Mainboard (board BRD4002A, available standalone as Si-MB4002A)

The Wireless Pro Kit Mainboard is a strict superset of the Wireless Starter Kit,
the two boards are pin compatible for all shared functionality.

Wireless Starter Kits and Wireless Pro Kits are kits that bundle one or more
Radio Boards with one or more Mainboards.

In Zephyr, Radio Boards are used as board targets, irrespective of whether the
board was acquired standalone or as part of a Starter Kit or Pro Kit. The kit
name of the standalone Radio Board is used as the board target.

Hardware
********

Wireless Starter Kit Mainboard:

- Advanced Energy Monitor providing real-time information about energy consumption at up to 10 ksps
- Packet Trace Interface
- Virtual COM port
- On-board Segger J-Link debugger with USB and Ethernet interfaces
- Ultra-low power 128x128 pixel memory LCD
- 2 user buttons and 2 LEDs
- 20 pin 2.54mm expansion header
- Si7021 Relative Humidity and Temperature Sensor
- Breakout pads for Wireless SoC I/O

Wireless Pro Kit Mainboard:

- Advanced Energy Monitor providing real-time information about energy consumption at up to 100 ksps
- Packet Trace Interface
- Virtual COM port
- On-board Segger J-Link debugger with USB and Ethernet interfaces
- Ultra-low power 128x128 pixel memory LCD
- 2 user buttons, joystick and 2 LEDs
- 20 pin 2.54mm expansion header
- Si7021 Relative Humidity and Temperature Sensor
- Breakout pads for Wireless SoC I/O
