.. zephyr:board:: adafruit_feather_esp32s2

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
- Built-in NeoPixel indicator RGB LED
- STEMMA QT connector for I2C devices, with switchable power for low-power mode

.. note::

   - The `Adafruit ESP32-S2 Feather with BME280 Sensor`_ is the same board as the
     `Adafruit ESP32-S2 Feather`_ but with an already equipped BME280 Sensor, but is not
     stated as a separate board, instead the BME280 needs to be added via a devicetree
     overlay. All boards, except the `Adafruit ESP32-S2 Feather with BME280 Sensor`_ have a
     space for it, but will not be shipped with.
   - As of May 31, 2023 - Adafruit has changed the battery monitor chip from the
     now-discontinued LC709203F to the MAX17048. Check the back silkscreen of your Feather to
     see which chip you have.
   - For the MAX17048 and LC709203F a driver in zephyr exists and is supported, but needs to be
     added via a devicetree overlay.
   - For the `Adafruit ESP32-S2 Feather`_ there are two different Revisions ``rev B`` and
     ``rev C``. The ``rev C`` board has revised the power circuitry for the NeoPixel and I2C
     QT port. Instead of a transistor the ``rev C`` has a LDO regulator. To enable the
     NeoPixel and I2C QT port on ``rev B`` boards ``GPIO7`` (``i2c_reg``) needs to be set to
     LOW and on ``rev C`` boards it needs to be set HIGH.

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

The `Adafruit ESP32-S2 Feather`_ User Guide has detailed information about the board including
pinouts and the schematic.

- `Adafruit ESP32-S2 Feather Pinouts`_
- `Adafruit ESP32-S2 Feather Schematic`_

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

**Rev B**

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: adafruit_feather_esp32s2@B
   :goals: build flash

**Rev C**

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: adafruit_feather_esp32s2@C
   :goals: build flash

Testing the NeoPixel
********************

There is a sample available to verify that the NeoPixel on the board are
functioning correctly with Zephyr:

**Rev B**

   .. zephyr-app-commands::
      :zephyr-app: samples/drivers/led/led_strip
      :board: adafruit_feather_esp32s2@B
      :goals: build flash

**Rev C**

   .. zephyr-app-commands::
      :zephyr-app: samples/drivers/led/led_strip
      :board: adafruit_feather_esp32s2@C
      :goals: build flash

Testing the Fuel Gauge
**********************

There is a sample available to verify that the MAX17048 or LC709203F fuel gauge on the board are
functioning correctly with Zephyr

.. note::
   As of May 31, 2023 Adafruit changed the battery monitor chip from the now-discontinued LC709203F
   to the MAX17048.

**Rev B**

For the Rev B a devicetree overlay for the LC709203F fuel gauge already exists in the
``samples/drivers/fuel_gauge/boards`` folder.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/fuel_gauge
   :board: adafruit_feather_esp32s2@B
   :goals: build flash

**Rev C**

For the Rev C a devicetree overlay for the MAX17048 fuel gauge already exists in the
``samples/drivers/fuel_gauge/boards`` folder.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/fuel_gauge
   :board: adafruit_feather_esp32s2@C
   :goals: build flash

For the LC709203F a devicetree overlay needs to be added to the build.
The overlay can be added via the ``--extra-dtc-overlay`` argument  and should most likely includes
the following:

.. code-block:: devicetree

   / {
      aliases {
         fuel-gauge0 = &lc709203f;
      };
   };

   &i2c0 {
      lc709203f: lc709203f@0b {
         compatible = "onnn,lc709203f";
         status = "okay";
         reg = <0x0b>;
         power-domains = <&i2c_reg>;
         apa = "500mAh";
         battery-profile = <0x01>;
      };
   };


.. zephyr-app-commands::
   :zephyr-app: samples/drivers/fuel_gauge
   :board: adafruit_feather_esp32s2@C
   :west-args: --extra-dtc-overlay="boards/name_of_your.overlay"
   :goals: build flash

Testing Wi-Fi
*************

There is a sample available to verify that the Wi-Fi on the board are
functioning correctly with Zephyr:

.. note::
   The Prerequisites must be met before testing Wi-Fi.

**Rev B**

   .. zephyr-app-commands::
      :zephyr-app: samples/net/wifi/shell
      :board: adafruit_feather_esp32s2@B
      :goals: build flash

**Rev C**

   .. zephyr-app-commands::
      :zephyr-app: samples/net/wifi/shell
      :board: adafruit_feather_esp32s2@C
      :goals: build flash

References
**********

.. target-notes::

.. _`Adafruit ESP32-S2 Feather`: https://www.adafruit.com/product/5000
.. _`Adafruit ESP32-S2 Feather with BME280 Sensor`: https://www.adafruit.com/product/5303
.. _`Adafruit ESP32-S2 Feather Pinouts`: https://learn.adafruit.com/adafruit-esp32-s2-feather/pinouts
.. _`Adafruit ESP32-S2 Feather Schematic`: https://learn.adafruit.com/adafruit-esp32-s2-feather/downloads
.. _`SparkFun Qwiic`: https://www.sparkfun.com/qwiic
.. _`STEMMA QT`: https://learn.adafruit.com/introducing-adafruit-stemma-qt
