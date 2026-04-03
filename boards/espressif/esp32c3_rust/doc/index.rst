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

.. include:: ../../../espressif/common/soc-esp32c3-features.rst
   :start-after: espressif-soc-esp32c3-features

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

System Requirements
*******************

.. include:: ../../../espressif/common/system-requirements.rst
   :start-after: espressif-system-requirements

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

.. include:: ../../../espressif/common/building-flashing.rst
   :start-after: espressif-building-flashing

.. include:: ../../../espressif/common/board-variants.rst
   :start-after: espressif-board-variants

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

References
**********

.. target-notes::

.. _`ESP32-C3-DevKit-RUST`: https://github.com/esp-rs/esp-rust-board/tree/v1.2
