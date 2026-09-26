.. zephyr:board:: esp32p4_wifi6_touch_lcd_4_3

Overview
********

The Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 is a multimedia development board built around
the ESP32-P4 SoC and a bonded 4.3 inch capacitive touch panel. It pairs 32 MB of stacked
PSRAM with 32 MB of NOR flash and adds a MicroSD slot, USB 2.0 OTG HS, an audio subsystem
and a 40-pin GPIO header. Wireless connectivity comes from an on-board ESP32-C6-MINI-1
module connected over SDIO.

Waveshare sells the same board as ESP32-P4-WIFI6-Touch-LCD-4.3 and as
ESP32-P4-WIFI6-Touch-LCD-4.3-C. The two share this PCB, including the MIPI CSI connector;
the ``-C`` part number only means that an OV5647 camera module is supplied with it.

This board definition provides both high-performance (HP) core and low-power (LP) core
targets. The Zephyr console is routed to UART0 (GPIO37/38), which is the port wired to the
Type-C USB-to-UART bridge.

Hardware
********

The board included peripherals:

- ESP32-P4NRW32 SoC (silicon revision v3.x) with 32 MB stacked PSRAM and 32 MB on-board
  NOR flash
- Type-C USB-to-UART port (CH343P) for power, flashing and serial console
- Type-C USB 2.0 OTG HS port on the SoC high-speed PHY
- 4.3 inch 480x800 IPS panel on a 2-lane MIPI DSI connector, driven by a Sitronix ST7701
  controller, with a GT911 5-point capacitive touch controller on the same FPC
- Backlight boost converter enabled by GPIO33, with GPIO26 available for PWM dimming
- 15-pin 2-lane MIPI CSI camera connector with the Raspberry Pi pinout
- ESP32-C6-MINI-1 Wi-Fi 6 and Bluetooth LE co-processor on SDIO (clk=18, cmd=19, d0=14,
  d1=15, d2=16, d3=17), enabled by GPIO54
- MicroSD card slot (4-bit SDHC at 40 MHz: clk=43, cmd=44, d0=39, d1=40, d2=41, d3=42)
- ES8311 audio codec with an NS4150B speaker amplifier enabled by GPIO53, an ES7210 echo
  canceller and a dual microphone array, all on I2S0 (mclk=13, bclk=12, ws=10, dout=9,
  din=11)
- I2C0 bus (SDA=GPIO7, SCL=GPIO8, 400 kHz), shared by the touch controller, the audio
  codec and the camera connector
- 40-pin GPIO expansion header carrying GPIO2-5, GPIO7, GPIO8, GPIO21, GPIO22, GPIO28-32,
  GPIO34, GPIO35, GPIO37, GPIO38, GPIO46-52 and both USB PHYs
- Boot (GPIO35) and reset buttons

All four internal LDO regulators are configured as always-on: ``ldo1`` and ``ldo4`` at
3.3 V, ``ldo2`` at 1.8 V, and ``ldo3`` at 2.5 V for the MIPI D-PHY.

The touch controller's interrupt line is not routed to the SoC (R35 is unpopulated), so
the GT911 driver polls it and picks the I2C address the panel straps. The camera
connector, the audio codec and the echo canceller have no driver in Zephyr and are not
enabled.

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

References
**********

.. target-notes::

.. _`ESP32-P4-WIFI6-Touch-LCD-4.3 Wiki`: https://docs.waveshare.com/ESP32-P4-WIFI6-Touch-LCD-4.3
.. _`ESP32-P4-WIFI6-Touch-LCD-4.3 Schematic`: https://files.waveshare.com/wiki/ESP32-P4-WIFI6-Touch-LCD-4.3/ESP32-P4-WIFI6-Touch-LCD-4.3-schematic.pdf
