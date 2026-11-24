.. zephyr:board:: esp32s3_touch_amoled_1_75c

Overview
********

The `ESP32-S3-Touch-AMOLED-1.75C`_ is an ESP32-S3 development board from Waveshare with a round
1.75 inch 466x466 AMOLED touch display. It has 32 MB of flash and 8 MB of octal PSRAM, Wi-Fi and
Bluetooth Low Energy, an AXP2101 power management IC with a battery charger, an ES8311 audio
codec and ES7210 microphone ADC with a speaker amplifier, and a QMI8658 6-axis IMU.

Hardware
********

- ESP32-S3R8 with 32 MB of quad SPI flash and 8 MB of octal PSRAM
- 1.75 inch 466x466 AMOLED with a CO5300 controller, on QSPI
- CST9217 capacitive touch controller
- AXP2101 power management IC with a Li-ion battery charger and fuel gauge
- ES8311 audio codec and ES7210 microphone ADC, with two microphones and a speaker amplifier
- QMI8658 6-axis IMU
- BOOT and PWR buttons

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

Supported Features
==================

.. zephyr:board-supported-hw::

The display is driven over single-line SPI on the panel's first QSPI data line. The CST9217 touch
controller, the ES7210 and audio over I2S are not supported yet.

Connections and IOs
===================

+------------+------------------------------------+
| GPIO       | Function                           |
+============+====================================+
| GPIO0      | BOOT button (active low)           |
+------------+------------------------------------+
| GPIO1      | Display reset                      |
+------------+------------------------------------+
| GPIO2      | Touch reset                        |
+------------+------------------------------------+
| GPIO3      | PWR button (active high)           |
+------------+------------------------------------+
| GPIO4-7    | Display QSPI data 0-3              |
+------------+------------------------------------+
| GPIO8      | I2S data out (ES8311)              |
+------------+------------------------------------+
| GPIO9      | I2S bit clock                      |
+------------+------------------------------------+
| GPIO10     | I2S data in (ES7210)               |
+------------+------------------------------------+
| GPIO11     | Touch interrupt                    |
+------------+------------------------------------+
| GPIO12     | Display chip select                |
+------------+------------------------------------+
| GPIO13     | Display tearing effect             |
+------------+------------------------------------+
| GPIO14     | I2C SCL                            |
+------------+------------------------------------+
| GPIO15     | I2C SDA                            |
+------------+------------------------------------+
| GPIO16     | I2S master clock                   |
+------------+------------------------------------+
| GPIO21     | IMU interrupt 1                    |
+------------+------------------------------------+
| GPIO38     | Display QSPI clock                 |
+------------+------------------------------------+
| GPIO45     | I2S word select                    |
+------------+------------------------------------+
| GPIO46     | Speaker amplifier enable           |
+------------+------------------------------------+

The I2C bus has the CST9217 (0x5A), AXP2101 (0x34), ES8311 (0x18), ES7210 (0x40) and
QMI8658 (0x6B). GPIO17, GPIO18 and GPIO39 to GPIO42 are on test pads on the back of the board.
See the `ESP32-S3-Touch-AMOLED-1.75C schematic`_ for details.

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

The board's OpenOCD configuration (``support/openocd.cfg``) sets ``ESP_ONLYCPU 1``, so it debugs
only the PROCPU (CPU0). The APPCPU (CPU1) is not exposed to the debugger by default.

References
**********

.. target-notes::

.. _`ESP32-S3-Touch-AMOLED-1.75C`: https://www.waveshare.com/esp32-s3-touch-amoled-1.75c.htm
.. _`ESP32-S3-Touch-AMOLED-1.75C schematic`: https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C/blob/main/hardware/schematic/ESP32-S3-Touch-AMOLED-1.75C-schematic.pdf
