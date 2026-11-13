.. zephyr:code-sample:: espressif-sdm
   :name: Espressif sigma-delta LED fade

   Fade an LED with the Espressif sigma-delta modulator.

Overview
********

This sample sweeps the pulse density of one sigma-delta channel between -90
and 90. Density zero is about a 50% PDM stream. Higher values spend more time
high, so an LED wired from the GPIO to ground grows brighter and then dims.

The channel sample rate is 1 MHz. Connect an LED and a series resistor between
the GPIO below and ground.

.. list-table::
   :header-rows: 1

   * - Board
     - GPIO
   * - ESP32-DevKitC
     - 4
   * - ESP32-S2-DevKitC
     - 4
   * - ESP32-S3-DevKitC
     - 4
   * - ESP32-C3-DevKitC
     - 4
   * - ESP32-C5-DevKitC-1
     - 4
   * - ESP32-C6-DevKitC-1
     - 18
   * - ESP32-H2-DevKitM-1
     - 4
   * - ESP32-P4-Function-EV-Board
     - 4

ESP32-C2 and ESP32-C61 have no sigma-delta modulator, so this sample does not
target their devkits.

Building and Running
********************

Build for the board you are using. Example for the ESP32-C6 DevKitC:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/espressif/sdm
   :board: esp32c6_devkitc/esp32c6/hpcore
   :goals: build
   :compact:
