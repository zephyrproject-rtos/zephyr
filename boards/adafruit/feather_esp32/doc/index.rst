.. zephyr:board:: adafruit_feather_esp32

Overview
********

The Adafruit ESP32 Feather is an ESP32-based development board using the
Feather standard layout.

It features the following integrated components:

- ESP32-PICO-V3-02 chip (240MHz dual core, Wi-Fi + BLE)
- 520KB SRAM
- USB-C port connected to USB to Serial converter
- LiPo battery connector and charger
- Charging indicator LED and user LED
- NeoPixel RGB LED
- Reset and user buttons
- STEMMA QT I2C connector

Supported Features
==================

.. zephyr:board-supported-hw::

System requirements
===================

Prerequisites
-------------

Espressif HAL requires WiFi and Bluetooth binary blobs in order to work. Run
the commands below to retrieve the files.

.. code-block:: shell

   west update
   west blobs fetch hal_espressif

Building & flashing
-------------------

Use the standard build and flash process for this board. See
:ref:`build_an_application` and :ref:`application_run` for more details.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: adafruit_feather_esp32/esp32/procpu
   :goals: build flash

The baud rate of 921600bps is set by default. If experiencing issues when flashing,
try using different values by using ``--esp-baud-rate <BAUD>`` option during
``west flash`` (e.g. ``west flash --esp-baud-rate 115200``).

After flashing, view the serial monitor with the espressif monitor command.

.. code-block:: shell

   west espressif monitor

Testing
=======

On-board LED
------------

Test the functionality of the user LED connected to pin 13 with the blinky
sample program.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: adafruit_feather_esp32/esp32/procpu
   :goals: build flash

NeoPixel
--------

Test the on-board NeoPixel using the led_strip sample program.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led/led_strip
   :board: adafruit_feather_esp32/esp32/procpu
   :goals: build flash

User button
-----------

Test the button labeled SW38 using the button input sample program.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/button
   :board: adafruit_feather_esp32/esp32/procpu
   :goals: build flash

Wi-Fi
-----

Test ESP32 Wi-Fi functionality using the Wi-Fi shell module.

.. note::
   The hal_espressif blobs must be fetched first.

.. zephyr-app-commands::
   :zephyr-app: samples/net/wifi/shell
   :board: adafruit_feather_esp32/esp32/procpu
   :goals: build flash

References
**********
- `Adafruit ESP32 Feather V2 <https://www.adafruit.com/product/5400>`_
- `Adafruit ESP32 Feather V2 Pinouts <https://learn.adafruit.com/adafruit-esp32-feather-v2/pinouts>`_
- `Adafruit ESP32 Feather V2 Schematic <https://learn.adafruit.com/adafruit-esp32-feather-v2/downloads#schematic-and-fab-print-3112284>`_
- `ESP32-PICO-MINI-02 Datasheet <https://cdn-learn.adafruit.com/assets/assets/000/109/588/original/esp32-pico-mini-02_datasheet_en.pdf?1646852017>`_ (PDF)
- `STEMMA QT <https://learn.adafruit.com/introducing-adafruit-stemma-qt>`_
