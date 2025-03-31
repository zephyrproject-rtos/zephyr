.. zephyr:board:: pico_plus2

Overview
********

The `Pimoroni Pico Plus 2`_ is a compact and versatile board featuring the Raspberry Pi RP2350B SoC.
It includes USB Type-C, Qwiic/STEMMA QT connectors, SP/CE connectors, a debug connector,
a reset button, and a BOOT button.

Hardware
********

- Dual Cortex-M33 or Hazard3 processors at up to 150MHz
- 520KB of SRAM, and 4MB of on-board flash memory
- 16MB of on-board QSPI flash (supports XiP)
- 8MB of PSRAM
- USB 1.1 with device and host support
- Low-power sleep and dormant modes
- Drag-and-drop programming using mass storage over USB
- 48 multi-function GPIO pins including 8 that can be used for ADC
- 2 SPI, 2 I2C, 2 UART, 3 12-bit 500ksps Analogue to Digital - Converter (ADC), 24 controllable PWM channels
- 2 Timer with 4 alarms, 1 AON Timer
- Temperature sensor
- 3 Programmable IO (PIO) blocks, 12 state machines total for custom peripheral support
- USB-C connector for power, programming, and data transfer
- Qwiic/STEMMA QT(Qw/ST) connector
- SP/CE connector
- 3-pin debug connector, this can use with `Raspberry Pi Debug Probe`_.
- Reset button and BOOT button (BOOT button also usable as a user switch)

Supported Features
==================

.. zephyr:board-supported-hw::

You can use peripherals that are made by using the PIO.
See :ref:`rpi_pico_pio_based_features`


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The overall explanation regarding flashing and debugging is the same as or ``rpi_pico``.
See :ref:`rpi_pico_flashing_using_openocd` and :ref:`rpi_pico_flashing_using_uf2`
in ``rpi_pico`` documentation.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: pico_plus2
   :goals: build flash
   :gen-args: -DOPENOCD=/usr/local/bin/openocd

.. target-notes::

.. _Pimoroni Pico Plus 2:
  https://shop.pimoroni.com/products/pimoroni-pico-plus-2

.. _Raspberry Pi Debug Probe:
   https://www.raspberrypi.com/documentation/microcontrollers/debug-probe.html
