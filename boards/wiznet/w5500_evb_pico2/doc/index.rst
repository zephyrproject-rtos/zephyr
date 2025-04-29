.. zephyr:board:: w5500_evb_pico2

Overview
********

W5500-EVB-Pico2 is a microcontroller evaluation board based on the Raspberry
Pi RP2350A and fully hardwired TCP/IP controller W5500 - and basically works
the same as Raspberry Pi Pico2 board but with additional Ethernet via W5500.
The USB bootloader allows the ability to flash without any adapter, in a
drag-and-drop manner. It is also possible to flash and debug the boards with
their SWD interface, using an external adapter.

Hardware
********

- Dual core Arm Cortex-M33 or Hazard3 processor running up to 133MHz
- 520KB on-chip SRAM
- 16MB on-board QSPI flash with XIP capabilities
- 26 GPIO pins
- 3 Analog inputs
- 2 UART peripherals
- 2 SPI controllers
- 2 I2C controllers
- 16 PWM channels
- USB 1.1 controller (host/device)
- 3 Programmable I/O (PIO) for custom peripherals
- On-board LED
- 1 Watchdog timer peripheral
- Wiznet W5500 Ethernet MAC/PHY

Supported Features
==================

.. zephyr:board-supported-hw::

Pin Mapping
===========

The peripherals of the RP2350A SoC can be routed to various pins on the board.
The configuration of these routes can be modified through DTS. Please refer to
the datasheet to see the possible routings for each peripheral.

External pin mapping on the W5500_EVB_PICO2 is identical to the Raspberry Pi
Pico2. Since GPIO 25 is routed to the on-board LED on, similar to the Raspberry
Pi Pico, the blinky example works as intended. The W5500 is routed to the SPI0
(P16-P19), with the reset and interrupt signal for the W5500 routed to P20 and
P21, respectively. All of these are shared with the edge connector on the
board.

Refer to `W55500 Evaluation Board Pico2 Documentation`_ for a board schematic and
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
- SPI0_RX : P16
- SPI0_CSN : P17
- SPI0_SCK : P18
- SPI0_TX : P19
- W5500 Reset : P20
- W5500 Interrupt : P21
- ADC_CH0 : P26
- ADC_CH1 : P27
- ADC_CH2 : P28
- ADC_CH3 : P29

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

Using OpenOCD
-------------

The overall explanation regarding flashing and debugging is the same as or
``rpi_pico``.
See :ref:`rpi_pico_flashing_using_openocd`. in ``rpi_pico`` documentation.

A typical build command for w5500_evb_pico2 is as follows.
This assumes a CMSIS-DAP adapter such as the Raspberry Pi Debug Probe,
but if you are using something else, specify ``RPI_PICO_DEBUG_ADAPTER``.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: w5500_evb_pico2
   :goals: build flash
   :gen-args: -DOPENOCD=/usr/local/bin/openocd

Using UF2
---------

If you don't have an SWD adapter, you can flash the Raspberry Pi Pico with
a UF2 file. By default, building an app for this board will generate a
:file:`build/zephyr/zephyr.uf2` file. If the Pico is powered on with the ``BOOTSEL``
button pressed, it will appear on the host as a mass storage device. The
UF2 file should be drag-and-dropped to the device, which will flash the Pico.

Debugging
=========

The SWD interface can also be used to debug the board. To achieve this, you can
either use SEGGER JLink or OpenOCD.

Using OpenOCD
-------------

Install OpenOCD as described for flashing the board.

Here is an example for debugging the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: w5500_evb_pico2
   :maybe-skip-config:
   :goals: debug
   :gen-args: -DOPENOCD=/usr/local/bin/openocd

.. target-notes::

.. _pico_setup.sh:
  https://raw.githubusercontent.com/raspberrypi/pico-setup/master/pico_setup.sh

.. _Getting Started with Raspberry Pi Pico:
  https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf

.. _W55500 Evaluation Board Pico2 Documentation:
  https://docs.wiznet.io/Product/iEthernet/W5500/w5500-evb-pico2
