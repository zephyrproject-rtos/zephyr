.. zephyr:board:: wio_wm1110_dev_kit

Overview
********

The Wio-WM1110 Dev Kit is based on the Wio-WM1110 Wireless Module. This module
integrates a LoRa® transceiver and a multi-purpose radio front-end for geolocation.
The LoRa® transceiver provides low-power network coverage, while GNSS (GPS/Beidou)
and Wi-Fi scanning offer location coverage while also providing connectivity options
for a variety of peripherals.

Hardware
********

Wio-WM1110 Dev Kit featuring:

- Accelerometer, LIS3DHTR
- Temperature and humidity sensor, SHT41
- Two user LEDs (green and red)
- Three buttons (two user and one reset)
- Three Grove connectors
- USB Type-C connector
- GNSS antenna connector
- LoRa antenna connector
- NFC antenna IPEX connector
- Support for solar and battery power

For more information about Wio-WM1110 Dev Kit: `Wio-WM1110 Dev Kit Product Page`_.

The Wio-WM1110 Wireless Module is embedded with Nordic Semiconductor nRF52840
and Semtech LR1110. In this module the nRF52840 is intended to be used as an
application processor and the LR1110 interfaces with it via SPI. Most of the
interfaces and pins exposed in this board are on nRF52840.

For more information about Wio-WM1110 Wireless Module: `Wio-WM1110 Wireless Module Product Page`_.

Nordic Semiconductor nRF52840 featuring:

- ARM® Cortex®-M4 32-bit processor with FPU, 64 MHz
- Bluetooth®5, IEEE 802.15.4-2006, 2.4 GHz transceiver
- 1 MB flash and 256 kB RAM
- ARM® TrustZone® Cryptocell 310 security subsystem
- Secure boot

For more information about Nordic Semiconductor nRF52840: `Nordic Semiconductor nRF52840 Product Page`_.

Semtech LR1110 featuring:

- Worldwide ISM frequency bands support in the range 150 - 960MHz
- GNSS (GPS/ BeiDou) low-power scanning
- 802.11b/g/n Wi-Fi ultra-low-power passive scanning
- 150 - 2700MHz continuous frequency synthesizer range
- Hardware support for AES-128 encryption/decryption based algorithms

For more information about Semtech LR1110: `Semtech LR1110 Product Page`_.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

+-------+----------+----------------------+
| Name  | Function | Usage                |
+=======+==========+======================+
| P0.13 | GPIO     | Green LED            |
+-------+----------+----------------------+
| P0.14 | GPIO     | Red LED              |
+-------+----------+----------------------+
| P0.23 | GPIO     | User Button 1 (BTN1) |
+-------+----------+----------------------+
| P0.25 | GPIO     | User Button 2 (BTN2) |
+-------+----------+----------------------+
| P0.18 | GPIO     | Reset Button (RST)   |
+-------+----------+----------------------+
| P0.22 | UART0_RX | Debug Console        |
+-------+----------+----------------------+
| P0.24 | UART0_TX | Debug Console        |
+-------+----------+----------------------+

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

.. note::
   The Wio-WM1110 Dev Kit does not include an on-board debug probe. But it can be
   flashed and debugged by connecting an external SWD debugger to the SWD header.

Flashing
========

You can build and flash an application in the usual way.

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: wio_wm1110_dev_kit
   :goals: build flash

Debugging
=========

You can debug an application in the usual way.

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: wio_wm1110_dev_kit
   :maybe-skip-config:
   :goals: debug

.. _Nordic Semiconductor nRF52840 Product Page:
   https://www.nordicsemi.com/Products/nRF52840

.. _Wio-WM1110 Dev Kit Product Page:
   https://www.seeedstudio.com/Wio-WM1110-Dev-Kit-p-5677.html

.. _Semtech LR1110 Product Page:
   https://www.semtech.com/products/wireless-rf/lora-edge/lr1110

.. _Wio-WM1110 Wireless Module Product Page:
   https://www.seeedstudio.com/Wio-WM1110-Module-LR1110-and-nRF52840-p-5676.html
