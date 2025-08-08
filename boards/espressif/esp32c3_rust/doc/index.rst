.. zephyr:board:: esp32c3_rust

Overview
********

ESP32-C3-DevKit-RUST is based on the ESP32-C3, a single-core Wi-Fi and Bluetooth 5 (LE) microcontroller SoC,
based on the open-source RISC-V architecture. This special board also includes the ESP32-C3-MINI-1 module,
a 6DoF IMU, a temperature and humidity sensor, a Li-Ion battery charger, and a Type-C USB. The board is designed
to be easily used in training sessions, demonstrating its capabilities with all the board peripherals.
For more information, check `ESP32-C3-DevKit-RUST`_.

Hardware
********

SoC Features:

- IEEE 802.11 b/g/n-compliant
- Bluetooth 5, Bluetooth mesh
- 32-bit RISC-V single-core processor, up to 160MHz
- 384 KB ROM
- 400 KB SRAM (16 KB for cache)
- 8 KB SRAM in RTC
- 22 x programmable GPIOs
- 3 x SPI
- 2 x UART
- 1 x I2C
- 1 x I2S
- 2 x 54-bit general-purpose timers
- 3 x watchdog timers
- 1 x 52-bit system timer
- Remote Control Peripheral (RMT)
- LED PWM controller (LEDC)
- Full-speed USB Serial/JTAG controller
- General DMA controller (GDMA)
- 1 x TWAI®
- 2 x 12-bit SAR ADCs, up to 6 channels
- 1 x temperature sensor

For more information, check the datasheet at `ESP32-C3 Datasheet`_ or the technical reference
manual at `ESP32-C3 Technical Reference Manual`_.

Supported Features
==================

.. zephyr:board-supported-hw::

I2C Peripherals
===============

This board includes the following peripherals over the I2C bus:

+---------------------------+--------------+---------+
| Peripheral                | Part number  | Address |
+===========================+==============+=========+
| IMU                       | ICM-42670-P  |  0x68   |
+---------------------------+--------------+---------+
| Temperature and Humidity  | SHTC3        |  0x70   |
+---------------------------+--------------+---------+

I2C Bus Connection
==================

+---------+--------+
| Signal  | GPIO   |
+=========+========+
| SDA     | GPIO10 |
+---------+--------+
| SCL     | GPIO8  |
+---------+--------+

I/Os
====

The following devices are connected through GPIO:

+--------------+--------+
| I/O Devices  | GPIO   |
+==============+========+
| WS2812 LED   | GPIO2  |
+--------------+--------+
| LED          | GPIO7  |
+--------------+--------+
| Button/Boot  | GPIO9  |
+--------------+--------+

Power
=====

* USB type-C (*no PD compatibility*).
* Li-Ion battery charger.

System requirements
*******************

Prerequisites
=============

Espressif HAL requires WiFi and Bluetooth binary blobs in order work. Run the command
below to retrieve those files.

.. code-block:: console

   west blobs fetch hal_espressif

.. note::

   It is recommended running the command above after :file:`west update`.

Building and Flashing
*********************

.. zephyr:board-supported-runners::

.. include:: ../../../espressif/common/building-flashing.rst
   :start-after: espressif-building-flashing

.. include:: ../../../espressif/common/board-variants.rst
   :start-after: espressif-board-variants

Debugging
*********

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

References
**********

.. target-notes::

.. _`ESP32-C3-DevKit-RUST`: https://github.com/esp-rs/esp-rust-board/tree/v1.2
.. _`ESP32-C3 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf
.. _`ESP32-C3 Technical Reference Manual`: https://espressif.com/sites/default/files/documentation/esp32-c3_technical_reference_manual_en.pdf
.. _`OpenOCD ESP32`: https://github.com/espressif/openocd-esp32/releases
