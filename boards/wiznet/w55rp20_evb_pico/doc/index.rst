.. zephyr:board:: w55rp20_evb_pico

Overview
********

W55RP20-EVB-Pico is a microcontroller evaluation board based on the W55RP20,
a SiP device that combines a Raspberry Pi RP2040 microcontroller and a fully
hardwired TCP/IP controller W5500. It basically works the same as a Raspberry
Pi Pico board, with additional Ethernet provided by the integrated W5500.
The USB bootloader allows the ability to flash without any adapter, in a
drag-and-drop manner. It is also possible to flash and debug the boards with
their SWD interface, using an external adapter.

Hardware
********
- Dual core Arm Cortex-M0+ processor running up to 133MHz
- 264KB on-chip SRAM
- 2MB on-board flash
- 22 GPIO pins
- 4 Analog inputs
- 2 UART peripherals
- 2 SPI controllers
- 2 I2C controllers
- 16 PWM channels
- USB 1.1 controller (host/device)
- 8 Programmable I/O (PIO) for custom peripherals
- On-board LED
- 1 Watchdog timer peripheral
- Wiznet W5500 Ethernet MAC/PHY

Supported Features
==================

.. zephyr:board-supported-hw::

Pin Mapping
===========

The peripherals of the RP2040 SoC can be routed to various pins on the board.
The configuration of these routes can be modified through DTS. Please refer to
the datasheet to see the possible routings for each peripheral.

External pin mapping on the W55RP20_EVB_PICO is identical to the Raspberry Pi
Pico. However, GPIO17, GPIO20, GPIO21, GPIO22, GPIO23, GPIO24, and GPIO25 are
not available on the edge connector due to internal connections on the board.
GPIO19 is routed to the on-board LED. The W5500 interface signals are
internally connected and are not exposed on the edge connector.

Refer to `W55RP20 Evaluation Board Documentation`_ for a board schematic and
other certifications.

Default Zephyr Peripheral Mapping:
----------------------------------

.. rst-class:: rst-columns

- UART0_TX : P0
- UART0_RX : P1
- I2C0_SDA : P4
- I2C0_SCL : P5
- I2C1_SDA : P14
- I2C1_SCL : P15
- W5500 interface : Internal (not exposed)
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
   :board: w55rp20_evb_pico
   :goals: build flash
   :flash-args: --openocd /usr/local/bin/openocd

.. target-notes::

.. _pico_setup.sh:
  https://raw.githubusercontent.com/raspberrypi/pico-setup/master/pico_setup.sh

.. _Getting Started with Raspberry Pi Pico:
  https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf

.. _W55RP20 Evaluation Board Documentation:
  https://docs.wiznet.io/Product/Chip/MCU/W55RP20/w55rp20-evb-pico
