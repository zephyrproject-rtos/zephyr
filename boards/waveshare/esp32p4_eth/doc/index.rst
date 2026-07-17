.. zephyr:board:: esp32p4_eth

Overview
********

ESP32-P4-ETH is a high-performance dual-core RISC-V development board based on
the ESP32-P4 chip designed by Waveshare. It supports MIPI-CSI and MIPI-DSI
interfaces, as well as common peripherals such as SPI, I2S, I2C, LED PWM, MCPWM,
RMT, ADC, UART, and TWAI\ :sup:`TM`. It also supports USB OTG 2.0 HS, Ethernet,
and SDIO Host 3.0 for high-speed connectivity.

The ESP32-P4 does not include an RF module (Wi-Fi-supporting ESP32-P4 designs
use an additional microcontroller such as an ESP32-C5 as a Wi-Fi/Bluetooth
interface), therefore this board does NOT have Wi-Fi/Bluetooth. For more
information, check `ESP32-P4-ETH`_.

This board ships with ESP32-P4 silicon revision v1.3 (a pre-v3 part), and the
Zephyr board configuration targets that revision with the HP cores clocked at
360 MHz. Pre-v3 silicon tops out at 360 MHz; v3.x parts run at up to 400 MHz.

Hardware
********

ESP32-P4-ETH ships with:

- ESP32-P4NRW32 SoC (silicon revision v1.3) with 32 MB PSRAM stacked in the
  package, paired with 32 MB of on-board NOR flash
- USB-C connector wired to a USB-to-UART bridge, accessing the ROM bootloader
  and console
- Additional USB header for native USB OTG 2.0 HS
- MIPI CSI camera connector
- MIPI DSI display connector
- microSD card slot
- 10/100 Mbps Ethernet RJ45 port driven by the on-chip EMAC and an
  IP101 PHY over RMII, with headers for a PoE module
- Microphone and 4 W power amplifier via an I2S audio codec
- Boot and reset buttons
- Expansion headers exposing UART, I2C, and GPIO

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

.. _`ESP32-P4-ETH`: https://www.waveshare.com/wiki/ESP32-P4-ETH
