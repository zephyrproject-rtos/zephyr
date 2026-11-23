.. zephyr:board:: esp32p4_wifi6

Overview
********

The Waveshare ESP32-P4-WIFI6 is a multimedia development board built around the
ESP32-P4 SoC. It stacks 32 MB of PSRAM and connects a 32 MB NOR flash, exposes
a Type-C USB-to-UART port for flashing and console, a 4-pin USB-OTG HS
connector, MIPI CSI/DSI FPC connectors, a MicroSD slot, and a 40-pin GPIO
header with the Raspberry Pi Pico pinout (compatible with Pico HATs). Wireless
connectivity is provided by an on-board ESP32-C6-MINI-1 module connected over
SDIO.

This board definition provides both high-performance (HP) core and low-power
(LP) core targets. The Zephyr console is routed to UART0 (GPIO37/38), which is
the port wired to the Type-C USB-to-UART bridge.

Hardware
********

The board included peripherals:

- ESP32-P4 SoC (silicon revision v1.3) with 32 MB stacked PSRAM and 32 MB
  on-board NOR flash
- Type-C USB-to-UART port (CH343P) for power, flashing and serial console
- 4-pin USB 2.0 OTG HS connector
- 2-lane MIPI CSI camera connector
- 2-lane MIPI DSI display connector
- MicroSD card slot (4-bit SDHC at 40 MHz: clk=43, cmd=44, d0=39, d1=40,
  d2=41, d3=42), powered through a GPIO45 load switch
- I2C0 bus (SDA=GPIO7, SCL=GPIO8, 400 kHz)
- 40-pin GPIO expansion header (Raspberry Pi Pico pinout)
- Boot (GPIO35) and reset buttons

The 32 MB flash uses a custom partition table with two 15.75 MB application
slots (``image-0`` / ``image-1``) for MCUboot and two 32 KB slots for the
LP core images, followed by a 192 KB ``storage`` partition; the
``image-scratch`` and ``coredump`` partitions are located at the end of the
flash.

Three internal LDO regulators are configured as always-on: ``ldo1`` and
``ldo4`` at 3.3 V, and ``ldo2`` at 1.8 V. MIPI DSI/CSI, I2S audio and the
ESP32-C6 SDIO wireless co-processor are not enabled in this initial board
port.

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

.. _`ESP32-P4-WIFI6 Wiki`: https://www.waveshare.com/wiki/ESP32-P4-WIFI6
.. _`ESP32-P4-WIFI6 Schematic`: https://files.waveshare.com/wiki/ESP32-P4-WIFI6/ESP32-P4-WIFI6-datasheet.pdf
