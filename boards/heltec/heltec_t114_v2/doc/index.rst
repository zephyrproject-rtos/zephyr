.. zephyr:board:: heltec_t114_v2

Overview
********

Heltec Mesh Node T114 (Rev. 2.0) is an IoT development board designed and produced by
Heltec Automation(TM). It combines an nRF52840 microcontroller with an SX1262 LoRa
transceiver, optional 1.14" TFT display, and GNSS support. It is compatible with
Meshtastic and LoRaWAN. See the `Heltec Mesh Node T114 pages`_ for more details.

The features include the following:

- Microprocessor: Nordic nRF52840 (ARM Cortex-M4F, 64 MHz, 256 KB RAM, 1 MB flash)
- LoRa node chip: Semtech SX1262
- Optional onboard 1.14" 135×240 TFT display (ST7789V, SPI)
- GNSS module interface (NMEA over UART)
- USB-C interface with USB device (CDC ACM) support
- BLE 5.0, IEEE 802.15.4 (OpenThread)
- One user button, one green LED, SK6812 RGB LED strip (2 LEDs)
- Li-Po battery & solar panel 2*1.25mm connector
- 2*13*2.54mm expansion pin headers

Supported Features
==================

.. zephyr:board-supported-hw::

See the `Heltec Mesh Node T114 pages`_ for a complete list of board hardware features.

Connections and IOs
===================

LED
---

* LED4 (green) = P1.03

Push buttons
------------

* BUTTON0 = SW1 = P1.10

System Requirements
*******************

- Host machine with USB port
- USB-C cable for programming and serial console (CDC ACM)
- Optional: J-Link or other SWD debugger for debugging
- Optional: UF2 bootloader for drag-and-drop flashing; build with the
  ``heltec_t114_v2/nrf52840/uf2`` board target

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The default ``heltec_t114_v2/nrf52840`` target uses the standard Nordic nRF52840
partition layout (full flash image, no Nordic nRF5 SDK bootloader / MBR link
offset). Use the ``heltec_t114_v2/nrf52840/uf2`` variant when building for the
board's UF2 bootloader and its application partition layout.

Applications for the ``heltec_t114_v2/nrf52840`` board configuration can be built,
flashed, and debugged in the usual way. See :ref:`build_an_application` and
:ref:`application_run` for more details on building and running.

Flashing
========

First, run your favorite terminal program to listen for output (e.g. 115200 baud).
Then build and flash. Example for the :zephyr:code-sample:`blinky` application:

.. code-block:: console

   west build -b heltec_t114_v2/nrf52840 samples/basic/blinky
   west flash

For the UF2 bootloader variant (drag-and-drop firmware):

.. code-block:: console

   west build -b heltec_t114_v2/nrf52840/uf2 samples/basic/blinky
   west flash

Debugging
=========

Debugging is supported via SWD. Use an nRF52-compatible probe (e.g. nRF52840 DK,
J-Link) and connect SWDIO, SWDCLK, GND, and VDD. Refer to the :ref:`nordic_segger`
page to learn about debugging Nordic boards with a Segger IC. OpenOCD and Zephyr's
debug runner can also be used with the appropriate board configuration.

References
**********

.. target-notes::

.. _`Heltec Mesh Node T114 pages`: https://heltec.org/project/mesh-node-t114/
.. _`Heltec Mesh Node T114 (Wiki)`: https://wiki.heltec.org/docs/devices/open-source-hardware/nrf52840-series/mesh-node-t114/
.. _`nRF52840 Product Specification`: https://docs.nordicsemi.com/bundle/ps_nrf52840/page/keyfeatures_html5.html
