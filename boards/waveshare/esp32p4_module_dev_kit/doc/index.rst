.. zephyr:board:: esp32p4_module_dev_kit

Overview
********

ESP32-P4-Module-DEV-KIT is a high‑performance dual‑core RISC‑V development board
based on the ESP32-P4 chip designed by Waveshare. It supports rich human‑machine
interaction interfaces, including MIPI‑CSI (with an integrated Image Signal
Processor, ISP) and MIPI‑DSI interfaces, as well as common peripherals such as
SPI, I2S, I2C, LED PWM, MCPWM, RMT, ADC, UART, and TWAI™. It also supports USB
OTG 2.0 HS, Ethernet, and SDIO Host 3.0 for high-speed connectivity.
For more information, check
`ESP32-P4-Module-DEV-KIT`_.

Hardware
********

ESP32-P4-Module-DEV-KIT ships with:

- ESP32-P4 SoC with 32 MB PSRAM stacked in the package, and 16 MB Nor Flash
integrated on the module
- Equipped with an ESP32-C6 WIFI/BT co-processor, expanding WIFI 6/Bluetooth
5 and other functions through SDIO
- USB-C connector wired to the on-chip USB Serial/JTAG controller
- USB-C connector for USB-OTG
- MIPI CSI camera connector
- MIPI DSI display connector
- microSD card slot
- 10/100 Mbps Ethernet RJ45 port driven by the on-chip EMAC and an
  IP101 PHY over RMII, with PoE module interface
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

.. _`ESP32-P4-Module-DEV-KIT`: https://docs.waveshare.com/ESP32-P4-Module-DEV-KIT
