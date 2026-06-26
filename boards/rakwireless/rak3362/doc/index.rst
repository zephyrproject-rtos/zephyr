.. zephyr:board:: rak3362

Overview
********

RAK3362 is a WisBlock Core module based on the RAK3162 LoRa module. It uses the
Nordic nRF54L15 SoC (Arm Cortex-M33) and integrates a Semtech SX1262 LoRa
transceiver. The module is designed for the RAK WisBlock ecosystem and should be
used with a WisBlock Base Board such as RAK19007 or RAK19010.

It provides Bluetooth Low Energy (BLE 5.4), LoRa, and multiple peripheral
interfaces (UART, I2C, SPI, ADC) through the WisBlock connector.

Hardware
********

- Nordic nRF54L15 Arm Cortex-M33 processor
- 1.4 MB flash, 188 KB SRAM
- Semtech SX1262 low power LoRa transceiver with TCXO
- UART x2, I2C, SPI x2, ADC, PWM, NFC
- SWD debug interface
- WisBlock IO, ADC, and PWM slot mappings

For more information about the module:

- `RAK3362 Product Page`_

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The RAK3362 module can be programmed and debugged via SWD using an external
debug probe (J-Link, pyOCD, or OpenOCD).

Building and Flashing
=====================

A WisBlock Base Board shield is recommended to expose console, LEDs, battery
measurement, and sensor slots.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rak3362/nrf54l15/cpuapp
   :shield: rakwireless_rak19007
   :goals: build flash

Debugging
=========

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rak3362/nrf54l15/cpuapp
   :shield: rakwireless_rak19007
   :goals: debug

References
**********

.. target-notes::

.. _RAK3362 Product Page:
   https://docs.rakwireless.com/product-categories/wisblock/rak3362/overview
