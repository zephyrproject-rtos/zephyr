.. zephyr:board:: adafruit_feather_esp32s2_tft

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
- LC709203F or MAX17048 fuel gauge for battery voltage and state-of-charge reporting
- Charging indicator LED, user LED, reset and boot buttons.
- Built-in NeoPixel indicator RGB LED
- STEMMA QT connector for I2C devices, with switchable power for low-power mode
- 240x135 pixel IPS TFT color display with 1.14" diagonal and ST7789 chipset

.. note::

   - The board has a space for a BME280, but will not be shipped with.
   - As of May 31, 2023 - Adafruit has changed the battery monitor chip from the
     now-discontinued LC709203F to the MAX17048. Check the back silkscreen of your Feather to
     see which chip you have.
   - For the MAX17048 and LC709203F a driver in zephyr exists and is supported, but needs to be
     added via a devicetree overlay.

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

The `Adafruit ESP32-S2 TFT Feather`_ User Guide has detailed information about the board including
pinouts and the schematic.

- `Adafruit ESP32-S2 TFT Feather Pinouts`_
- `Adafruit ESP32-S2 TFT Feather Schematic`_

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

Testing the NeoPixel
********************

There is a sample available to verify that the NeoPixel on the board are
functioning correctly with Zephyr:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led/led_strip
   :board: adafruit_feather_esp32s2_tft
   :goals: build flash

Testing the TFT
***************

.. note::
   To activate the backlight of the display ``GPIO45`` (``backlight``) needs to be set to HIGH.
   This will be done automatically via ``board_late_init_hook()``.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/lvgl
   :board: adafruit_feather_esp32s2_tft
   :goals: build flash

Testing the Fuel Gauge
**********************

There is a sample available to verify that the MAX17048 or LC709203F fuel gauge on the board are
functioning correctly with Zephyr.

.. note::
   As of May 31, 2023 Adafruit changed the battery monitor chip from the now-discontinued LC709203F
   to the MAX17048.

**LC709203F Fuel Gauge**

For the LC709203F a devicetree overlay already exists in the ``samples/drivers/fuel_gauge/boards`` folder.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/fuel_gauge
   :board: adafruit_feather_esp32s2_tft
   :goals: build flash

**MAX17048 Fuel Gauge**

For the MAX17048 a devicetree overlay needs to be added to the build.
The overlay can be added via the ``--extra-dtc-overlay`` argument  and should most likely includes
the following:

.. code-block:: devicetree

   / {
      aliases {
         fuel-gauge0 = &max17048;
      };
   };

   &i2c0 {
      max17048: max17048@36 {
         compatible = "maxim,max17048";
         status = "okay";
         reg = <0x36 >;
         power-domains = <&i2c_reg>;
      };
   };

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/fuel_gauge
   :board: adafruit_feather_esp32s2_tft
   :west-args: --extra-dtc-overlay="boards/name_of_your.overlay"
   :goals: build flash

Testing Wi-Fi
*************

There is a sample available to verify that the Wi-Fi on the board are
functioning correctly with Zephyr:

.. note::
   The Prerequisites must be met before testing Wi-Fi.

.. zephyr-app-commands::
   :zephyr-app: samples/net/wifi/shell
   :board: adafruit_feather_esp32s2_tft
   :goals: build flash

References
**********

.. target-notes::

.. _`Adafruit ESP32-S2 TFT Feather`: https://www.adafruit.com/product/5300
.. _`Adafruit ESP32-S2 TFT Feather Pinouts`: https://learn.adafruit.com/adafruit-esp32-s2-tft-feather/pinouts
.. _`Adafruit ESP32-S2 TFT Feather Schematic`: https://learn.adafruit.com/adafruit-esp32-s2-tft-feather/downloads
.. _`SparkFun Qwiic`: https://www.sparkfun.com/qwiic
.. _`STEMMA QT`: https://learn.adafruit.com/introducing-adafruit-stemma-qt
