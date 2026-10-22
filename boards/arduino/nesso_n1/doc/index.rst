.. zephyr:board:: arduino_nesso_n1

Overview
********

The Arduino Nesso N1 is a compact multi-protocol IoT development board co-designed by Arduino and
M5Stack and built around the Espressif ESP32-C6 32-bit RISC-V SoC running at up to 160 MHz. It
combines Wi-Fi 6, Bluetooth LE 5.3, IEEE 802.15.4 (Thread/Zigbee) and sub-GHz LoRa connectivity in a
single device. For more information, check `Arduino Nesso N1`_.

Hardware
********

The board carries a Bosch BMI270 6-axis IMU on the system I2C bus and a Semtech SX1262 LoRa
transceiver on SPI, alongside a 1.14" color TFT display, capacitive touch, IR transmitter,
programmable buttons and a 250 mAh LiPo battery managed by an on-board fuel gauge. USB-C is wired to
the ESP32-C6 native USB-Serial-JTAG so flashing and the serial console share a single cable, with no
external programmer needed.

.. include:: ../../../espressif/common/soc-esp32c6-features.rst
   :start-after: espressif-soc-esp32c6-features

Supported Features
==================

.. zephyr:board-supported-hw::

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

.. _`Arduino Nesso N1`: https://docs.arduino.cc/hardware/nesso-n1/
