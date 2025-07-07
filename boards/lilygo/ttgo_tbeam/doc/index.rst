.. zephyr:board:: ttgo_tbeam

Overview
********

The Lilygo TTGO TBeam, is an ESP32-based development board for LoRa applications.

It's available in two versions supporting two different frequency ranges and features the following integrated components:

- ESP32-PICO-D4 chip (240MHz dual core, 600 DMIPS, 520KB SRAM, Wi-Fi)
- SSD1306, 128x64 px, 0.96" screen (optional)
- SX1278 (433MHz) or SX1276 (868/915/923MHz) LoRa radio frontend (optional, with SMA or IPEX connector)
- NEO-6M or NEO-M8N GNSS module
- X-Powers AXP2101 PMIC
- JST GH 2-pin battery connector
- 18650 Li-Ion battery clip

Some of the ESP32 I/O pins are accessible on the board's pin headers.

Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

Start Application Development
*****************************

Before powering up your Lilygo TTGO TBeam, please make sure that the board is in good
condition with no obvious signs of damage.

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
bootstrap the board with the ESP32-PICO-D4 SoC.

To build the sample application using sysbuild use the command:

.. zephyr-app-commands::
   :tool: west
   :zephyr-app: samples/hello_world
   :board: ttgo_tbeam/esp32/procpu
   :goals: build
   :west-args: --sysbuild
   :compact:

By default, the ESP32-PICO-D4 sysbuild creates bootloader (MCUboot) and application
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
   :board: ttgo_tbeam/esp32/procpu
   :goals: build

The usual ``flash`` target will work with the ``ttgo_tbeam`` board target.
Here is an example for the :zephyr:code-sample:`hello_world`
application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ttgo_tbeam/esp32/procpu
   :goals: flash

The default baud rate for the Lilygo TTGO TBeam is set to 1500000bps. If experiencing issues when flashing,
try using different values by using ``--esp-baud-rate <BAUD>`` option during
``west flash`` (e.g. ``west flash --esp-baud-rate 115200``).

You can also open the serial monitor using the following command:

.. code-block:: shell

   west espressif monitor

After the board has automatically reset and booted, you should see the following
message in the monitor:

.. code-block:: console

   ***** Booting Zephyr OS vx.x.x-xxx-gxxxxxxxxxxxx *****
   Hello World! ttgo_tbeam/esp32/procpu

Code samples
============

The following sample applications will work out of the box with this board:

* :zephyr:code-sample:`lora-send`
* :zephyr:code-sample:`lora-receive`
* :zephyr:code-sample:`gnss`
* :zephyr:code-sample:`wifi-shell`
* :zephyr:code-sample:`character-frame-buffer`
* :zephyr:code-sample:`blinky`

Debugging
*********

Lilygo TTGO TBeam debugging is not supported due to pinout limitations.

Related Documents
*****************
- `Lilygo TTGO TBeam schematic <https://github.com/Xinyuan-LilyGO/LilyGo-LoRa-Series/blob/master/schematic/LilyGo_TBeam_V1.2.pdf>`_ (PDF)
- `Lilygo TTGO TBeam documentation <https://www.lilygo.cc/products/t-beam-v1-1-esp32-lora-module>`_
- `Lilygo github repo <https://github.com/Xinyuan-LilyGo>`_
- `ESP32-PICO-D4 Datasheet <https://www.espressif.com/sites/default/files/documentation/esp32-pico-d4_datasheet_en.pdf>`_ (PDF)
- `ESP32 Datasheet <https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf>`_ (PDF)
- `ESP32 Hardware Reference <https://docs.espressif.com/projects/esp-idf/en/latest/esp32/hw-reference/index.html>`_
- `SX127x Datasheet <https://www.semtech.com/products/wireless-rf/lora-connect/sx1276#documentation>`_
- `SSD1306 Datasheet <https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf>`_ (PDF)
- `NEO-6M Datasheet <https://content.u-blox.com/sites/default/files/products/documents/NEO-6_DataSheet_%28GPS.G6-HW-09005%29.pdf>`_ (PDF)
- `NEO-N8M Datasheet <https://content.u-blox.com/sites/default/files/NEO-M8-FW3_DataSheet_UBX-15031086.pdf>`_ (PDF)
