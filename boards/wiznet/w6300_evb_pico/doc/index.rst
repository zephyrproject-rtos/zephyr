.. zephyr:board:: w6300_evb_pico

Overview
********

W6300-EVB-Pico is a microcontroller evaluation board based on the Raspberry Pi
RP2040 and the WIZnet W6300 Ethernet controller. It follows the Raspberry Pi
Pico form factor and pinout, while adding Ethernet through a PIO-driven QSPI
interface to the W6300.

The USB bootloader allows the board to be flashed without an external adapter,
in a drag-and-drop manner. It is also possible to flash and debug the board
through its SWD interface with an external debug probe.

Hardware
********

- Dual core Arm Cortex-M0+ processor running up to 133 MHz
- 264 KB on-chip SRAM
- 2 MB on-board QSPI flash with XIP capabilities
- 26 GPIO pins
- 3 analog inputs
- 2 UART peripherals
- 2 I2C controllers
- 16 PWM channels
- USB 1.1 controller (host/device)
- 2 Programmable I/O (PIO) blocks for custom peripherals
- On-board LED
- 1 watchdog timer peripheral
- WIZnet W6300 Ethernet controller connected through PIO QSPI

Supported Features
==================

.. zephyr:board-supported-hw::

Pin Mapping
===========

The peripherals of the RP2040 SoC can be routed to various pins on the board.
The configuration of these routes can be modified through DTS.

External pin mapping on W6300-EVB-Pico is identical to the Raspberry Pi Pico.
GPIO 25 is routed to the on-board LED, so the :zephyr:code-sample:`blinky`
sample works as expected.

The W6300 is connected to a PIO-based QSPI bus using GPIO 16 through GPIO 22,
with the interrupt signal on GPIO 15. These signals are shared with the board's
edge connector.

Default Zephyr Peripheral Mapping:
----------------------------------

.. rst-class:: rst-columns

- UART0_TX : P0
- UART0_RX : P1
- I2C0_SDA : P4
- I2C0_SCL : P5
- W6300 Interrupt : P15
- W6300 CS : P16
- W6300 SCK : P17
- W6300 IO0 : P18
- W6300 IO1 : P19
- W6300 IO2 : P20
- W6300 IO3 : P21
- W6300 Reset : P22
- ADC_CH0 : P26
- ADC_CH1 : P27
- ADC_CH2 : P28

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The overall flashing and debugging flow is the same as for
:zephyr:board:`rpi_pico`. See
:ref:`rpi_pico_programming_and_debugging` in the :zephyr:board:`rpi_pico`
documentation. OpenOCD support requires a Raspberry Pi Pico compatible OpenOCD
setup.

Below is an example of building and flashing the :zephyr:code-sample:`blinky`
application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: w6300_evb_pico
   :goals: build flash
   :gen-args: -DRPI_PICO_DEBUG_ADAPTER=cmsis-dap
   :flash-args: --openocd /usr/local/bin/openocd
