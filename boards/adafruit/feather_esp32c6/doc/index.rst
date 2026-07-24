.. zephyr:board:: adafruit_feather_esp32c6

Overview
********

The Adafruit Feather ESP32-C6 boards are ESP32-C6 development boards in the
Feather standard layout, sharing peripheral placement with other devices labeled
as Feathers or FeatherWings. The board is equipped with an ESP32-C6 mini module,
a LiPo battery charger, a fuel gauge, a USB-C and `SparkFun Qwiic`_-compatible
`STEMMA QT`_ connector for the I2C bus.

Hardware
********

- ESP32-C6-MINI module, featuring the 32-bit core RISC-V, with a clock speed of up to 160 MHz
- 512 KB of internal HP SRAM, 16 KB LP SRAM, 4 MB flash
- USB-C directly connected to the ESP32-C6 for USB
- LiPo connector and built-in battery charging when powered via USB-C
- MAX17048 fuel gauge for battery voltage and state-of-charge reporting
- Built-in NeoPixel indicator RGB LED
- STEMMA QT connector for I2C devices, with switchable power for low-power mode

.. include:: ../../../espressif/common/soc-esp32c6-features.rst
   :start-after: espressif-soc-esp32c6-features

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The `Adafruit ESP32-C6 Feather`_ User Guide has detailed information about the board including
pinouts and the schematic.

- `Adafruit ESP32-C6 Feather Pinouts`_
- `Adafruit ESP32-C6 Feather Schematic`_

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
   :board: adafruit_feather_esp32c6/esp32c6/hpcore
   :goals: build flash

Testing the NeoPixel
********************

There is a sample available to verify that the NeoPixel on the board is
functioning correctly with Zephyr:


.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led/led_strip
   :board: adafruit_feather_esp32c6/esp32c6/hpcore
   :goals: build flash

Testing the Fuel Gauge
**********************

There is a sample available to verify that the MAX17048 fuel gauge on the board is
functioning correctly with Zephyr:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/fuel_gauge
   :board: adafruit_feather_esp32c6/esp32c6/hpcore
   :goals: build flash

Testing Wi-Fi
*************

There is a sample available to verify that Wi-Fi on the board is
functioning correctly with Zephyr:

.. note::
   The Prerequisites must be met before testing Wi-Fi.

.. zephyr-app-commands::
   :zephyr-app: samples/net/wifi/shell
   :board: adafruit_feather_esp32c6/esp32c6/hpcore
   :goals: build flash

References
**********

.. target-notes::

.. _`Adafruit ESP32-C6 Feather`: https://www.adafruit.com/product/5933
.. _`Adafruit ESP32-C6 Feather Pinouts`: https://learn.adafruit.com/adafruit-esp32-c6-feather/pinouts
.. _`Adafruit ESP32-C6 Feather Schematic`: https://learn.adafruit.com/adafruit-esp32-c6-feather/downloads
.. _`SparkFun Qwiic`: https://www.sparkfun.com/qwiic
.. _`STEMMA QT`: https://learn.adafruit.com/introducing-adafruit-stemma-qt
