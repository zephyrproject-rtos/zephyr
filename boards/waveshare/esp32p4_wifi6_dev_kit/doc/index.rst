.. zephyr:board:: esp32p4_wifi6_dev_kit

Overview
********

The Waveshare ESP32-P4-WIFI6-Dev-Kit is a multimedia development board built
around the ESP32-P4 SoC. The board pairs 32 MB of stacked PSRAM with 16 MB
of NOR flash and provides wired connectivity (USB, 100M Ethernet, MicroSD),
audio, MIPI CSI/DSI interfaces and a 40-pin GPIO header. Wireless
connectivity comes from an on-board ESP32-C6 module connected over SDIO.

This board definition targets the ESP32-P4 high-performance (HP) core.

Hardware
********

The board included peripherals:

- ESP32-P4 SoC (silicon revision v3.1) with 32 MB stacked PSRAM and 16 MB
  on-board NOR flash
- Type-C USB-to-UART port (CH343P) for power, flashing and serial console
- Two stacked USB-A ports on the USB OTG 2.0 HS controller (CH334F hub),
  host/device function switchable via jumper
- 100M Ethernet RJ45 port, with a reserved PoE module header
- 2-lane MIPI CSI camera connector
- 2-lane MIPI DSI display connector
- ES8311 audio codec with speaker PA, microphone and 3.5mm headphone jack
- MicroSD card slot (4-bit SDHC at 40 MHz: clk=43, cmd=44, d0=39, d1=40,
  d2=41, d3=42), powered through a GPIO45 load switch
- I2C0 bus (SDA=GPIO7, SCL=GPIO8, 400 kHz), shared with the audio codec and
  the CSI/DSI connectors; I2C and I3C peripheral headers
- RTC battery holder (rechargeable RTC battery only)
- 40-pin GPIO expansion header
- Boot (GPIO35) and reset buttons

Three internal LDO regulators are configured as always-on: ``ldo1`` and
``ldo4`` at 3.3 V, and ``ldo2`` at 1.8 V. MIPI DSI/CSI, I2S audio, Ethernet
and the ESP32-C6 SDIO wireless co-processor are not enabled in this initial
board port.

.. include:: ../../../espressif/common/soc-esp32p4-features.rst
   :start-after: espressif-soc-esp32p4-features

Supported Features
==================

.. zephyr:board-supported-hw::

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

.. _`Waveshare ESP32-P4-WIFI6-Dev-Kit Wiki`: https://docs.waveshare.com/ESP32-P4-WIFI6-DEV-KIT
.. _`Waveshare ESP32-P4-WIFI6-Dev-Kit Schematic`: https://www.waveshare.net/w/upload/3/39/ESP32-P4-WIFI6-DEV-KIT-datasheet.pdf
