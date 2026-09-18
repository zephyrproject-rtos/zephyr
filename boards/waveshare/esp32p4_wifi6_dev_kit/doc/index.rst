.. zephyr:board:: esp32p4_wifi6_dev_kit

Overview
********

The Waveshare ESP32-P4-WIFI6-Dev-Kit is a multimedia development board built
around the ESP32-P4 SoC. The board pairs 32 MB of stacked PSRAM with 16 MB
of NOR flash and provides wired connectivity (USB, 100M Ethernet, MicroSD),
audio, MIPI CSI/DSI interfaces and a 40-pin GPIO header. Wireless
connectivity comes from an on-board ESP32-C6 module connected over SDIO.

This board definition provides both high-performance (HP) core and
low-power (LP) core targets.

Hardware
********

The board included peripherals:

- ESP32-P4 SoC (silicon revision v3.1) with 32 MB stacked PSRAM and 16 MB
  on-board NOR flash
- Type-C USB-to-UART port (CH343P) for power, flashing and serial console
- Two stacked USB-A ports on the USB OTG 2.0 HS controller (CH334F hub),
  host/device function switchable via jumper
- 100M Ethernet RJ45 port with an IC+ IP101GR PHY (RMII: clk=50, tx_en=49,
  txd0=34, txd1=35, crs_dv=28, rxd0=29, rxd1=30; SMI: mdc=31, mdio=52;
  reset=GPIO51, PHY address 1), with a reserved PoE module header
- On-board ESP32-C6 acting as a Wi-Fi and Bluetooth radio co-processor,
  reached over SDIO
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
``ldo4`` at 3.3 V, and ``ldo2`` at 1.8 V. MIPI DSI/CSI and I2S audio.

Wi-Fi and Bluetooth
===================

The ESP32-P4 has no radio of its own. Wireless connectivity is provided by the
on-board ESP32-C6, which runs the esp-hosted-mcu co-processor firmware and is
reached over SDIO. On this board the ESP32-C6 sits on SDIO slot 1 of the SDMMC
controller and the microSD socket on slot 0; the two slots share the single
controller, which serialises transactions between them. The ESP32-C6 reset line
is driven by a host GPIO.

The Zephyr esp-hosted-mcu driver exposes the co-processor as a standard Wi-Fi
interface and, when Bluetooth is enabled, as an HCI controller, both carried
over the same SDIO link. Because the radio firmware runs on a separate chip, the
ESP32-C6 must be flashed with an esp-hosted-mcu firmware build whose major
version matches the one the host driver expects (currently the 3.x line; see the
``ESP_HOSTED_MCU_FW_VERSION_*`` Kconfig options). The driver queries the running
firmware version at start-up and logs a warning when the major version differs.

The co-processor firmware, the supported chipsets and transports, and the
protocol design are documented in the upstream `ESP-Hosted-MCU`_ project.

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
.. _`ESP-Hosted-MCU`: https://github.com/espressif/esp-hosted-mcu
