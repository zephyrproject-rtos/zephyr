.. zephyr:board:: rp2040_zero

Overview
********

RP2040-Zero, A Low-Cost, High-Performance Pico-Like MCU Board Based On Raspberry Pi Microcontroller RP2040.

Hardware
********
- RP2040 microcontroller chip designed by Raspberry Pi in the United Kingdom.
- Dual-core Arm Cortex M0+ processor, flexible clock running up to 133 MHz.
- 264KB of SRAM, and 2MB of on-board Flash memory.
- USB-C connector, keeps it up to date, easier to use.
- The castellated module allows soldering direct to carrier boards.
- USB 1.1 with device and host support.
- Low-power sleep and dormant modes.
- Drag-and-drop programming using mass storage over USB.
- 29 × multi-function GPIO pins (20× via edge pinout, others via solder points).
- 2 × SPI, 2 × I2C, 2 × UART, 4 × 12-bit ADC, 16 × controllable PWM channels.
- Accurate clock and timer on-chip.
- Temperature sensor.
- Accelerated floating-point libraries on-chip.
- 8 × Programmable I/O (PIO) state machines for custom peripheral support.

Supported Features
==================

.. zephyr:board-supported-hw::

Pin Mapping
===========

The peripherals of the RP2040 SoC can be routed to various pins on the board. The configuration of these routes can be modified through DTS. Please refer to the datasheet to see the possible routings for each peripheral.

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

Flashing
========

Using UF2
---------

Here is an example of building the sample for driving the built-in RGB led.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led/led_strip
   :board: rp2040_zero
   :goals: build
   :compact:

You must flash the RP2040-Zero with an UF2 file. One option is to use West (Zephyr’s meta-tool). To enter the UF2 flashing mode just keep the ``BOOT`` button pressed while you connect the USB port, it will appear on the host as a mass storage device. In alternative with the board already connected via USB you can keep the ``RESET`` button pressed, press and release ``BOOT``, release ``RESET``. At this point you can flash the image file by running:

.. code-block:: bash

  west flash

In alternative you can locate the generated file at ``build/zephyr/zephyr.uf2 file`` and simply drag-and-drop to the device after entreing the UF2 flashing mode.

References
**********

- `Official Documentation`_
- `WS2812 datasheet`_

.. _Official Documentation: https://www.waveshare.com/wiki/RP2040-Zero
.. _WS2812 datasheet: https://cdn-shop.adafruit.com/datasheets/WS2812.pdf
