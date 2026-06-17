.. zephyr:board:: esp32p4_eth
Overview
********

ESP32-P4-ETH is a high‑performance dual‑core RISC‑V development board
based on the ESP32-P4 chip designed by Waveshare. It supports rich human‑machine
interaction interfaces, including MIPI‑CSI (with an integrated Image Signal
Processor, ISP) and MIPI‑DSI interfaces, as well as common peripherals such as
SPI, I2S, I2C, LED PWM, MCPWM, RMT, ADC, UART, and TWAI™. It also supports USB
OTG 2.0 HS, Ethernet, and SDIO Host 3.0 for high-speed connectivity.
The ESP32-P4 does not include an RF module (WiFi-supporting ESP32-P4 modules
use an additional microcontroller e.g. ESP32-C5 as a WiFi/Bluetooth interface)
therefore this board does NOT have WiFi/Bluetooth.
For more information, check
`ESP32-P4-ETH`_.

Hardware
********

ESP32-P4-ETH ships with:

- ESP32-P4-NRW32 SoC with 32 MB PSRAM stacked in the package,
and 32MB onboard Nor Flash on the board
- USB-C connector wired to usb-uart chip, accessing ROM bootloader and console
- Additional USB header for native USB-OTG
- MIPI CSI camera connector
- MIPI DSI display connector
- microSD card slot
- 10/100 Mbps Ethernet RJ45 port driven by the on-chip EMAC and an
  IP101 PHY over RMII, with headers for PoE module
- Microphone and 4W power amplifier via I2S audio codec
- Boot and reset buttons
- Expansion headers exposing UART, I2C, SPI and GPIO

.. include:: ../../../espressif/common/soc-esp32p4-features.rst
   :start-after: espressif-soc-esp32p4-features

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

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

References
**********

.. target-notes::

.. _`ESP32-P4-ETH`: https://docs.waveshare.com/ESP32-P4-ETH
