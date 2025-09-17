.. zephyr:board:: pico_spe

Overview
********

The Pico-SPE is a small, low-cost, versatile boards from
KWS Computersysteme Gmbh. They are equipped with an RP2040 SoC, an on-board LED,
a USB connector, an SWD interface. The Pico-SPE additionally contains an
Microchip LAN8651 10Base-T1S module. The USB bootloader allows the
ability to flash without any adapter, in a drag-and-drop manner.
It is also possible to flash and debug the boards with their SWD interface,
using an external adapter.

Hardware
********
- Dual core Arm Cortex-M0+ processor running up to 133MHz
- 264KB on-chip SRAM
- 16MB on-board QSPI flash with XIP capabilities
- 16 GPIO pins
- 3 Analog inputs
- 2 UART peripherals
- 2 I2C controllers
- 16 PWM channels
- USB 1.1 controller (host/device)
- 8 Programmable I/O (PIO) for custom peripherals
- On-board LED
- 1 Watchdog timer peripheral
- Microchip LAN8651 10Base-T1S

Supported Features
==================

.. zephyr:board-supported-hw::

Pin Mapping
===========

The peripherals of the RP2040 SoC can be routed to various pins on the board.
The configuration of these routes can be modified through DTS. Please refer to
the datasheet to see the possible routings for each peripheral.

External pin mapping on the Pico-SPE is identical to the Pico, but note that internal
RP2040 GPIO lines 10, 11, 12, 13, 20, 21 are routed to the Microchip LAN8651 on the
Pico-SPE.

Default Zephyr Peripheral Mapping:
----------------------------------

.. rst-class:: rst-columns

- UART0_TX : P0
- UART0_RX : P1
- I2C0_SDA : P4
- I2C0_SCL : P5
- I2C1_SDA : P6
- I2C1_SCL : P7
- ADC_CH0 : P26
- ADC_CH1 : P27
- ADC_CH2 : P28

Programmable I/O (PIO)
**********************
The RP2040 SoC comes with two PIO periherals. These are two simple
co-processors that are designed for I/O operations. The PIOs run
a custom instruction set, generated from a custom assembly language.
PIO programs are assembled using :command:`pioasm`, a tool provided by Raspberry Pi.

Zephyr does not (currently) assemble PIO programs. Rather, they should be
manually assembled and embedded in source code. An example of how this is done
can be found at :zephyr_file:`drivers/serial/uart_rpi_pico_pio.c`.

Sample: SPI via PIO
====================

The :zephyr:code-sample:`bme280` sample includes a
demonstration of using the PIO SPI driver to communicate with an
environmental sensor.  The PIO SPI driver supports using any
combination of GPIO pins for an SPI bus, as well as allowing up to
four independent SPI buses on a single board (using the two SPI
devices as well as both PIO devices).

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The SWD interface can be used to program and debug the device,
e.g. using OpenOCD with the `Raspberry Pi Debug Probe <https://www.raspberrypi.com/documentation/microcontrollers/debug-probe.html>`_ .

The overall explanation regarding flashing and debugging is the same as for :zephyr:board:`rpi_pico`.
Refer to :ref:`rpi_pico_programming_and_debugging` for more information. N.b. OpenOCD support requires using Raspberry Pi's forked version of OpenOCD.

Below is an example of building and flashing the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: pico_spe
   :goals: build flash
   :flash-args: --openocd /usr/local/bin/openocd

.. target-notes::

.. _pico_setup.sh:
   https://raw.githubusercontent.com/raspberrypi/pico-setup/master/pico_setup.sh

.. _Getting Started with Pico-SPE-Series:
   https://kws-computer.de/go/pico-spe-getting-started

.. _Pico-SPE Documentation:
   https://kws-computer.de/go/pico-spe-datasheet
