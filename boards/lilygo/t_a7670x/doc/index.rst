.. zephyr:board:: t_a7670x

Overview
********

LilyGo T-A7670X is an LTE Cat 1 development board based on the Espressif
ESP32-WROVER-E module and a SIMCom A7670X cellular modem.

It features the following integrated components:

- ESP32 chip (240MHz dual core, 520KB SRAM, Wi-Fi, Bluetooth)
- SIMCom A7670X LTE Cat 1 / GSM modem
- Nano SIM card slot
- TF (microSD) card slot
- USB Type-C connector for power, programming and console
- 18650 battery holder with charger
- Solar panel input
- BOOT and RESET buttons
- 2 x 16-pin 2.54 mm expansion headers

Hardware
********

This board is based on the ESP32-WROVER-E module with 4MB of flash, 8MB of
PSRAM, Wi-Fi and BLE support.

The modem is fitted in one of the following regional variants, which share
the same PCB and pinout:

- A7670E: LTE-FDD B1/B3/B5/B8/B20, GSM 900/1800 MHz
- A7670SA: LTE-FDD B1/B2/B3/B4/B5/B7/B8/B28/B66, GSM 850/900/1800/1900 MHz
- A7670G: LTE-FDD and LTE-TDD global bands, GSM 850/900/1800/1900 MHz

.. include:: ../../../espressif/common/soc-esp32-features.rst
   :start-after: espressif-soc-esp32-features

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

+---------+------------------------------------------+
| GPIO    | Function                                 |
+=========+==========================================+
| GPIO0   | BOOT button (``sw0``)                    |
+---------+------------------------------------------+
| GPIO4   | Modem PWRKEY                             |
+---------+------------------------------------------+
| GPIO5   | Modem RESET                              |
+---------+------------------------------------------+
| GPIO12  | Modem power supply enable                |
+---------+------------------------------------------+
| GPIO25  | Modem DTR                                |
+---------+------------------------------------------+
| GPIO26  | Modem RXD (UART1 TX)                     |
+---------+------------------------------------------+
| GPIO27  | Modem TXD (UART1 RX)                     |
+---------+------------------------------------------+
| GPIO33  | Modem RI                                 |
+---------+------------------------------------------+
| GPIO2   | TF card MISO (SPI2)                      |
+---------+------------------------------------------+
| GPIO13  | TF card CS (SPI2)                        |
+---------+------------------------------------------+
| GPIO14  | TF card SCLK (SPI2)                      |
+---------+------------------------------------------+
| GPIO15  | TF card MOSI (SPI2)                      |
+---------+------------------------------------------+
| GPIO35  | Battery voltage (ADC1 channel 7)         |
+---------+------------------------------------------+
| GPIO36  | Solar voltage (ADC1 channel 0, V1.4 only)|
+---------+------------------------------------------+

The modem is described by the ``modem`` devicetree node, using the
``simcom,a76xx`` compatible on UART1. GPIO12 switches the modem supply and is
configured as a GPIO hog, so the modem is powered as soon as the GPIO driver
is initialized. GPIO12 is also the flash voltage strapping pin of the ESP32,
so it must not be driven high during reset.

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

Sample Applications
*******************

The following code samples will run out of the box on the T-A7670X board:

* :zephyr:code-sample:`cellular-modem`
* :zephyr:code-sample:`button`
* :zephyr:code-sample:`fs`
* :zephyr:code-sample:`wifi-shell`

To build the cellular modem sample:

.. zephyr-app-commands::
   :tool: west
   :app: samples/net/cellular_modem
   :board: t_a7670x/esp32/procpu
   :goals: build

References
**********

.. target-notes::

.. _`LilyGo T-A7670X wiki`: https://wiki.lilygo.cc/products/t-sim-series/t-a7670/
.. _`LilyGo T-A7670X schematic`: https://github.com/Xinyuan-LilyGO/LilyGO-T-A76XX/blob/main/schematic/esp32/T-A7670X-V1.4.pdf
.. _`LilyGo T-A76XX repository`: https://github.com/Xinyuan-LilyGO/LilyGO-T-A76XX
.. _`Espressif ESP32-WROVER-E datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-wrover-e_esp32-wrover-ie_datasheet_en.pdf
