.. zephyr:board:: adafruit_feather_esp32s2_tft_reverse

Overview
********

The Adafruit Feather ESP32-S2 boards are ESP32-S2 development boards in the
Feather standard layout, sharing peripheral placement with other devices labeled
as Feathers or FeatherWings. The board is equipped with an ESP32-S2 mini module,
a LiPo battery charger, a fuel gauge, a USB-C and `SparkFun Qwiic`_-compatible
`STEMMA QT`_ connector for the I2C bus.

Hardware
********

- ESP32-S2 mini module, featuring the 240MHz Tensilica processor
- 320KB SRAM, 4MB flash + 2MB PSRAM
- USB-C directly connected to the ESP32-S2 for USB
- LiPo connector and built-in battery charging when powered via USB-C
- MAX17048 fuel gauge for battery voltage and state-of-charge reporting
- Charging indicator LED, user LED, reset and boot buttons and has 2 additional buttons.
- Built-in NeoPixel indicator RGB LED
- 240x135 pixel IPS TFT color display with 1.14" diagonal and ST7789 chipset.

.. note::

   - The board has a space for a BME280, but will not be shipped with.
   - For the MAX17048 a driver in zephyr exists and is supported, but needs to be added via
     a devicetree overlay.

.. include:: ../../../espressif/common/soc-esp32-features.rst
   :start-after: espressif-soc-esp32-features

Supported Features
==================

.. zephyr:board-supported-hw::

.. note::
   USB-OTG is until now not supported. To see a serial output a FTDI-USB-RS232 or similar
   needs to be connected to the RX/TX pins on the feather connector.

Connections and IOs
===================

The `Adafruit ESP32-S2 Reverse TFT Feather`_ User Guide has detailed information about the board
including pinouts and the schematic.

- `Adafruit ESP32-S2 Reverse TFT Feather Pinouts`_
- `Adafruit ESP32-S2 Reverse TFT Feather Schematic`_

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

Testing the On-Board-LED
************************

There is a sample available to verify that the LEDs on the board are
functioning correctly with Zephyr:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: adafruit_feather_esp32s2_tft_reverse
   :goals: build flash

Testing the NeoPixel
********************

There is a sample available to verify that the NeoPixel on the board are
functioning correctly with Zephyr:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led/led_strip
   :board: adafruit_feather_esp32s2_tft_reverse
   :goals: build flash

Testing the TFT
***************

There is a sample available to verify that the TFT on the board are
functioning correctly with Zephyr:

.. note::
   To activated the backlight of the display ``GPIO45`` (``backlight``) needs to be set to HIGH.
   This will be done automatically via ``board_late_init_hook()``.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/lvgl
   :board: adafruit_feather_esp32s2_tft_reverse
   :goals: build flash

Testing the Fuel Gauge
**********************

There is a sample available to verify that the MAX17048 fuel gauge on the board are
functioning correctly with Zephyr:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/fuel_gauge
   :board: adafruit_feather_esp32s2_tft_reverse
   :goals: build flash

Testing Wi-Fi
*************

There is a sample available to verify that the Wi-Fi on the board are
functioning correctly with Zephyr:

.. note::
   The Prerequisites must be met before testing Wi-Fi.

.. zephyr-app-commands::
   :zephyr-app: samples/net/wifi/shell
   :board: adafruit_feather_esp32s2_tft_reverse
   :goals: build flash

References
**********

.. target-notes::

.. _`Adafruit ESP32-S2 Reverse TFT Feather`: https://www.adafruit.com/product/5345
.. _`Adafruit ESP32-S2 Reverse TFT Feather Pinouts`: https://learn.adafruit.com/esp32-s2-reverse-tft-feather/pinouts
.. _`Adafruit ESP32-S2 Reverse TFT Feather Schematic`: https://learn.adafruit.com/esp32-s2-reverse-tft-feather/downloads
.. _`SparkFun Qwiic`: https://www.sparkfun.com/qwiic
.. _`STEMMA QT`: https://learn.adafruit.com/introducing-adafruit-stemma-qt
