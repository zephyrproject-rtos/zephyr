.. zephyr:board:: ttgo_t7v1_5

Overview
********

LILYGO® TTGO T7 Mini32 V1.5 ia an IoT mini development board
based on the Espressif ESP32-WROVER-E module.

It features the following integrated components:
- ESP32 chip (240MHz dual core, 520KB SRAM, Wi-Fi, Bluetooth)
- on board antenna
- Micro-USB connector for power and communication
- JST GH 2-pin battery connector
- LED

Hardware
********

This board is based on the ESP32-WROVER-E module with 4MB of flash (there
are models 16MB as well), WiFi and BLE support. It has a Micro-USB port for
programming and debugging, integrated battery charging and an on-board antenna.

Supported Features
==================

.. zephyr:board-supported-hw::

System requirements
*******************

Prerequisites
=============

Espressif HAL requires WiFi and Bluetooth binary blobs in order to work. Run the command
below to retrieve those files.

.. code-block:: console

   west blobs fetch hal_espressif

.. note::

   It is recommended running the command above after :file:`west update`.

Building & Flashing
*******************

.. zephyr:board-supported-runners::

Simple boot
===========

The board could be loaded using a single binary image, without 2nd stage bootloader.
It is the default option when building the application without additional configuration.

.. note::

   Simple boot does not provide any security features nor OTA updates.

MCUboot bootloader
==================

User may choose to use MCUboot bootloader instead. In that case the bootloader
must be built (and flashed) at least once.

There are two options to be used when building an application:

1. Sysbuild
2. Manual build

.. note::

   User can select the MCUboot bootloader by adding the following line
   to the board default configuration file.

   .. code-block:: cfg

      CONFIG_BOOTLOADER_MCUBOOT=y

Sysbuild
========

The sysbuild makes it possible to build and flash all necessary images needed to
bootstrap the board with the ESP32 SoC.

To build the sample application using sysbuild use the command:

.. zephyr-app-commands::
   :tool: west
   :app: samples/hello_world
   :board: ttgo_t7v1_5/esp32/procpu
   :goals: build
   :west-args: --sysbuild
   :compact:

By default, the ESP32 sysbuild creates bootloader (MCUboot) and application
images. But it can be configured to create other kind of images.

Build directory structure created by sysbuild is different from traditional
Zephyr build. Output is structured by the domain subdirectories:

.. code-block::

  build/
  ├── hello_world
  │   └── zephyr
  │       ├── zephyr.elf
  │       └── zephyr.bin
  ├── mcuboot
  │    └── zephyr
  │       ├── zephyr.elf
  │       └── zephyr.bin
  └── domains.yaml

.. note::

   With ``--sysbuild`` option the bootloader will be re-build and re-flash
   every time the pristine build is used.

For more information about the system build please read the :ref:`sysbuild` documentation.

Manual build
============

During the development cycle, it is intended to build & flash as quickly possible.
For that reason, images can be built one at a time using traditional build.

The instructions following are relevant for both manual build and sysbuild.
The only difference is the structure of the build directory.

.. note::

   Remember that bootloader (MCUboot) needs to be flash at least once.

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ttgo_t7v1_5/esp32/procpu
   :goals: build

The usual ``flash`` target will work with the ``ttgo_t7v1_5`` board
configuration. Here is an example for the :zephyr:code-sample:`hello_world`
application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ttgo_t7v1_5/esp32/procpu
   :goals: flash

The default baud rate for the Lilygo TTGO T7 V1.5 is set to 1500000bps. If experiencing issues when flashing,
try using different values by using ``--esp-baud-rate <BAUD>`` option during
``west flash`` (e.g. ``west flash --esp-baud-rate 115200``).

You can also open the serial monitor using the following command:

.. code-block:: shell

   west espressif monitor

After the board has automatically reset and booted, you should see the following
message in the monitor:

.. code-block:: console

   ***** Booting Zephyr OS vx.x.x-xxx-gxxxxxxxxxxxx *****
   Hello World! ttgo_t7v1_5

Sample applications
===================

The following samples will run out of the box on the TTGO T7 V1.5 board.

To build the blinky sample:

.. zephyr-app-commands::
   :tool: west
   :app: samples/basic/blinky
   :board: ttgo_t7v1_5/esp32/procpu
   :goals: build

To build the bluetooth beacon sample:

.. zephyr-app-commands::
   :tool: west
   :app: samples/bluetooth/beacon
   :board: ttgo_t7v1_5/esp32/procpu
   :goals: build


Related Documents
*****************
.. _`Lilygo TTGO T7-V1.5 schematic`: https://github.com/LilyGO/TTGO-T7-Demo/blob/master/t7_v1.5.pdf
.. _`Lilygo github repo`: https://github.com/LilyGO/TTGO-T7-Demo/tree/master
.. _`Espressif ESP32-WROVER-E datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-wrover-e_esp32-wrover-ie_datasheet_en.pdf
.. _`OpenOCD ESP32`: https://github.com/espressif/openocd-esp32/releases
