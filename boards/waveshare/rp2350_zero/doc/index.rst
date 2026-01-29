.. zephyr:board:: rp2350_zero

Overview
********

RP2350-Zero, A Low-Cost, High-Performance Pico-Like MCU Board Based On Raspberry Pi Microcontroller RP2350.

Hardware
********
- RP2350A microcontroller chip designed by Raspberry Pi in the United Kingdom
- Adopts unique dual-core and dual-architecture design: dual-core Arm Cortex-M33 processor and dual-core Hazard 3 RISC-V processor, flexible clock running up to 150 MHz
- 520KB of SRAM, and 4MB of on-board Flash memory
- Uses a modern and convenient USB Type-C connector
- Castellated module allows soldering directly to carrier boards
- USB 1.1 with device and host support
- Low-power sleep and dormant modes
- Drag-and-drop programming using mass storage over USB
- 29 × multi-function GPIO pins (20× via edge pinout, others via solder points)
- 2 × SPI, 2 × I2C, 2 × UART, 4 × 12-bit ADC, 24 × controllable PWM channels
- Accurate on-chip clock and timer peripherals
- Temperature sensor
- Accelerated floating-point libraries on-chip
- 12 × Programmable I/O (PIO) state machines for custom peripheral support

Supported Features
==================

.. zephyr:board-supported-hw::

Pin Mapping
===========

The peripherals of the RP2350 SoC can be routed to various pins on the board. The configuration of these routes can be modified through DTS. Please refer to the datasheet to see the possible routings for each peripheral.

For detailed hardware information, see the `RP2350-Zero – Waveshare Wiki <https://www.waveshare.com/wiki/RP2350-Zero>`_.


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
- ADC_CH3 : P29

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

Using UF2
---------

Here is an example of building the sample for driving the built-in RGB led.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led/led_strip
   :board: rp2350_zero/rp2350a/m33
   :goals: build
   :compact:

You can flash the RP2350-Zero with an UF2 file. One option is to use West (Zephyr’s meta-tool). To enter the UF2 flashing mode just keep the ``BOOT`` button pressed while you connect the USB port, it will appear on the host as a mass storage device. Alternatively, with the board already connected via USB you can keep the ``BOOT`` button pressed, press and release ``RESET``, release ``BOOT``. At this point you can flash the image file by running:

.. code-block:: bash

  west flash

Alternatively, you can locate the generated file at ``build/zephyr/zephyr.uf2 file`` and simply drag-and-drop to the device after entering the UF2 flashing mode.

References
**********

- `Official Documentation`_
- `WS2812 datasheet`_

.. _Official Documentation: https://www.waveshare.com/wiki/RP2350-Zero
.. _WS2812 datasheet: https://cdn-shop.adafruit.com/datasheets/WS2812.pdf
