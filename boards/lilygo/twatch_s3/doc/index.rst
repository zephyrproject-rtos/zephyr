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

Building & Flashing
*******************

.. zephyr:board-supported-runners::

Prerequisites
=============

Espressif HAL requires WiFi and Bluetooth binary blobs in order to work. Run the command
below to retrieve those files.

.. code-block:: console

   west blobs fetch hal_espressif

.. note::

   It is recommended running the command above after :file:`west update`.

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
--------

The sysbuild makes it possible to build and flash all necessary images needed to
bootstrap the board with the ESP32 SoC.

To build the sample application using sysbuild, use the command:

.. zephyr-app-commands::
   :tool: west
   :zephyr-app: samples/hello_world
   :board: twatch_s3/esp32s3/procpu
   :goals: build
   :west-args: --sysbuild
   :compact:

By default, the ESP32-S3 sysbuild creates bootloader (MCUboot) and application
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

   With ``--sysbuild`` option the bootloader will be re-built and re-flashed
   every time the pristine build is used.

For more information about the system build please read the :ref:`sysbuild` documentation.

Manual build
------------

During the development cycle, it is intended to build & flash as quickly as possible.
For that reason, images can be built one at a time using traditional build.

The instructions following are relevant for both manual build and sysbuild.
The only difference is the structure of the build directory.

.. note::

   Remember that bootloader (MCUboot) needs to be flashed at least once.

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: twatch_s3/esp32s3/procpu
   :goals: build

The usual ``flash`` target will work with the ``twatch_s3`` board target
configuration. Here is an example for the :zephyr:code-sample:`hello_world`
application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: twatch_s3/esp32s3/procpu
   :goals: flash

The default baud rate is set to 1500000bps. If experiencing issues when flashing,
try using different values by using ``--esp-baud-rate <BAUD>`` option during
``west flash`` (e.g. ``west flash --esp-baud-rate 115200``).

You can also open the serial monitor using the following command:

.. code-block:: shell

   west espressif monitor

After the board has automatically reset and booted, you should see the following
message in the monitor:

.. code-block:: console

   ***** Booting Zephyr OS vx.x.x-xxx-gxxxxxxxxxxxx *****
   Hello World! twatch_s3/esp32s3/procpu

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
