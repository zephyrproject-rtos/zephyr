.. zephyr:board:: w6300_evb_pico2

Overview
********

W6300-EVB-Pico2 is a microcontroller evaluation board based on the Raspberry
Pi RP2350 and the fully hardwired TCP/IP controller W6300. It basically works
the same as a Raspberry Pi Pico 2 board, with additional Ethernet provided by
the W6300. The USB bootloader allows the ability to flash without any adapter,
in a drag-and-drop manner. It is also possible to flash and debug the boards
with their SWD interface, using an external adapter.

Hardware
********
- Dual Arm Cortex-M33 processor running up to 150MHz
- 520KB on-chip SRAM
- 2MB on-board QSPI flash
- 26 GPIO pins
- 4 Analog inputs
- 2 UART peripherals
- 2 SPI controllers
- 2 I2C controllers
- 24 PWM channels
- USB 1.1 controller (host/device)
- 12 Programmable I/O (PIO) state machines for custom peripherals
- On-board LED
- 1 Watchdog timer peripheral
- Wiznet W6300 Ethernet MAC/PHY with hardwired TCP/IP stack
- QSPI interface between RP2350 and W6300 (Single/Dual/Quad mode supported)

Supported Features
==================

.. zephyr:board-supported-hw::

Pin Mapping
===========

The peripherals of the RP2350 SoC can be routed to various pins on the board.
The configuration of these routes can be modified through DTS. Please refer to
the datasheet to see the possible routings for each peripheral.

External pin mapping on the W6300-EVB-Pico2 is identical to the Raspberry Pi
Pico 2. However, GPIO15, GPIO16, GPIO17, GPIO18, GPIO19, GPIO20, GPIO21, and
GPIO22 are internally connected to the W6300 and cannot be used for other
purposes when Ethernet is enabled. GPIO25 is routed to the on-board LED. The
W6300 interface signals are internally connected and are not exposed on the
edge connector.

Refer to `W6300-EVB-Pico2 Evaluation Board Documentation`_ for a board
schematic and other certifications.

Default Zephyr Peripheral Mapping:
----------------------------------

.. rst-class:: rst-columns

- UART0_TX : P0
- UART0_RX : P1
- I2C0_SDA : P4
- I2C0_SCL : P5
- I2C1_SDA : P14
- I2C1_SCL : P15
- W6300 INTn : P15 (internal)
- W6300 CSn : P16 (internal)
- W6300 SCLK : P17 (internal)
- W6300 IO0 (MOSI) : P18 (internal)
- W6300 IO1 (MISO) : P19 (internal)
- W6300 IO2 : P20 (internal)
- W6300 IO3 : P21 (internal)
- W6300 RSTn : P22 (internal)
- ADC_CH0 : P26
- ADC_CH1 : P27
- ADC_CH2 : P28
- ADC_CH3 : P29

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The overall explanation regarding flashing and debugging is the same as or :zephyr:board:`rpi_pico`.
See :ref:`rpi_pico_programming_and_debugging` in :zephyr:board:`rpi_pico` documentation. N.b. OpenOCD support requires using Raspberry Pi's forked version of OpenOCD.

Below is an example of building and flashing the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: w6300_evb_pico2
   :goals: build flash
   :flash-args: --openocd /usr/local/bin/openocd

.. target-notes::

.. _pico_setup.sh:
  https://raw.githubusercontent.com/raspberrypi/pico-setup/master/pico_setup.sh

.. _Getting Started with Raspberry Pi Pico:
  https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf

.. _W6300-EVB-Pico2 Evaluation Board Documentation:
  https://docs.wiznet.io/Product/Chip/MCU/W6300/w6300-evb-pico2
