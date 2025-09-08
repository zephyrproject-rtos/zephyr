.. zephyr:board:: twatch_s3

Overview
********

LILYGO T-Watch S3 is an ESP32-S3 based smartwatch with the following features:

- ESP32-S3-R8 chip

   - Dual core Xtensa LX-7 up to 240MHz
   - 8 MB of integrated PSRAM
   - Bluetooth LE v5.0
   - Wi-Fi 802.11 b/g/n

- 16 MB external QSPI flash (Winbond W25Q128JWPIQ)
- Power Management Unit (X-Powers AXP2101) which provides

   - Regulators (DC-DCs and LDOs)
   - Battery charging
   - Fuel gauge

- 470 mAh battery
- RTC (NXP PCF8563)
- Haptic (Texas Instruments DRV2605)
- Accelerometer (Bosch BMA423)
- 240x240 pixels LCD with touchscreen

   - ST7789V LCD Controller
   - Focaltech FT5336 touch sensor

- Microphone (Knowles SPM1423HM4H-B)
- LoRA radio (Semtech SX1262)
- Audio amplifier (Maxim MAX98357A)

The board features a single micro USB connector which can be used for serial
flashing, debugging and console thanks to the integrated JTAG support in the
chip.

It does not have any GPIO that can easily be connected to something external.
There is only 1 physical button which is connected to the PMU and it's used
to turn on/off the device.

Supported Features
==================

.. zephyr:board-supported-hw::

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

References
**********

.. target-notes::

.. _`Lilygo Twatch S3 schematic`: https://github.com/Xinyuan-LilyGO/TTGO_TWatch_Library/blob/t-watch-s3/schematic/T_WATCH_S3.pdf
.. _`Lilygo T-Watch S3 repo`: https://github.com/Xinyuan-LilyGO/TTGO_TWatch_Library/tree/t-watch-s3
.. _`Lilygo T-Watch Deps repo`: https://github.com/Xinyuan-LilyGO/T-Watch-Deps
.. _`ESP32-S3 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf
.. _`ESP32-S3 Technical Reference Manual`: https://www.espressif.com/sites/default/files/documentation/esp32-c3_technical_reference_manual_en.pdf
.. _`OpenOCD ESP32`: https://github.com/espressif/openocd-esp32/releases
.. _`JTAG debugging for ESP32-S3`: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/jtag-debugging/
