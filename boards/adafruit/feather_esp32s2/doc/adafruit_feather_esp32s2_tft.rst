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
- LC709203 or MAX17048 fuel gauge for battery voltage and state-of-charge reporting
- Charging indicator LED, user LED, reset and boot buttons.
- Built-in NeoPixel indicator RGB LED
- STEMMA QT connector for I2C devices, with switchable power for low-power mode
- 240x135 pixel IPS TFT color display with 1.14" diagonal and ST7789 chipset

.. note::

   - The board has a space for a BME280, but will not be shipped with.
   - As of May 31, 2023 - Adafruit has changed the battery monitor chip from the
     now-discontinued LC709203 to the MAX17048. Check the back silkscreen of your Feather to
     see which chip you have.
   - For the MAX17048 a driver in zephyr exists and is supported, but needs to be added via
     a devicetree overlay.
   - For the LC709203 a driver does'nt exists yet and the fuel gauge for boards with this IC
     is not available.

Supported Features
==================

.. zephyr:board-supported-hw::

.. note::
   USB-OTG is until now not supported see `ESP32 development overview`_. To see a serial output
   a FTDI-USB-RS232 or similar needs to be connected to the RX/TX pins on the feather connector.

Connections and IOs
===================

The `Adafruit ESP32-S2 TFT Feather`_ User Guide has detailed information about the board including
pinouts and the schematic.

- `Adafruit ESP32-S2 TFT Feather Pinouts`_
- `Adafruit ESP32-S2 TFT Feather Schematic`_

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Prerequisites
=============

Espressif HAL requires WiFi binary blobs in order work. Run the command below
to retrieve those files.

.. code-block:: console

   west update
   west blobs fetch hal_espressif

Building & Flashing
*******************

Simple boot
===========

The board could be loaded using the single binary image, without 2nd stage
bootloader. It is the default option when building the application without
additional configuration.

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

   .. code:: cfg

      CONFIG_BOOTLOADER_MCUBOOT=y

Sysbuild
========

The sysbuild makes possible to build and flash all necessary images needed to
bootstrap the board with the ESP32 SoC.

To build the sample application using sysbuild use the command:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: adafruit_feather_esp32s2_tft
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
   │   └── zephyr
   │       ├── zephyr.elf
   │       └── zephyr.bin
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

Build and flash applications as usual:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: adafruit_feather_esp32s2_tft
   :goals: build

The usual ``flash`` target will work. Here is an example for the :zephyr:code-sample:`hello_world`
application.

To enter ROM bootloader mode, hold down ``boot-button`` while clicking reset button.
When in the ROM bootloader, you can upload code and query the chip using ``west flash``.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: adafruit_feather_esp32s2_tft
   :goals: flash

After the flashing you will receive most likely this Error:

.. code-block:: console

   WARNING: ESP32-S2FNR2 (revision v0.0) chip was placed into download mode using GPIO0.
   esptool.py can not exit the download mode over USB. To run the app, reset the chip manually.
   To suppress this note, set --after option to 'no_reset'.
   FATAL ERROR: command exited with status 1: ...

As stated in the Warning-Message ``esptool`` can't reset the board by itself and this message
can be ignored and the board needs to be reseted via the Reset-Button manually.

Open the serial monitor using the following command:

.. code-block:: shell

   west espressif monitor

After the board has been manually reseted and booted, you should see the following
message in the monitor:

.. code-block:: console

   ***** Booting Zephyr OS vx.x.x-xxx-gxxxxxxxxxxxx *****
   Hello World! adafruit_feather_esp32s2_tft

Debugging
*********

ESP32-S2 support on OpenOCD is available at `OpenOCD`_.

ESP32-S2 has a built-in JTAG circuitry and can be debugged without any
additional chip. Only an USB cable connected to the D+/D- pins is necessary.

Further documentation can be obtained from the SoC vendor
in `JTAG debugging for ESP32-S2`_.

You can debug an application in the usual way. Here is an example for
the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: adafruit_feather_esp32s2_tft
   :goals: debug

Testing the On-Board-LED
************************

There is a sample available to verify that the LEDs on the board are
functioning correctly with Zephyr:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: adafruit_feather_esp32s2_tft
   :goals: build flash

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

Testing the Fuel Gauge (MAX17048)
*********************************

There is a sample available to verify that the MAX17048 fuel gauge on the board are
functioning correctly with Zephyr:

.. note::
   As of May 31, 2023 Adafruit changed the battery monitor chip from the now-discontinued LC709203
   to the MAX17048.

.. zephyr-app-commands::
   :zephyr-app: samples/fuel_gauge/max17048/
   :board: adafruit_feather_esp32s2_tft
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
.. _`OpenOCD`: https://github.com/openocd-org/openocd
.. _`ESP32 development overview`: https://github.com/zephyrproject-rtos/zephyr/issues/29394#issuecomment-2635037831
.. _`Adafruit ESP32-S2 TFT Feather Pinouts`: https://learn.adafruit.com/adafruit-esp32-s2-tft-feather/pinouts
.. _`Adafruit ESP32-S2 TFT Feather Schematic`: https://learn.adafruit.com/adafruit-esp32-s2-tft-feather/downloads
.. _`SparkFun Qwiic`: https://www.sparkfun.com/qwiic
.. _`STEMMA QT`: https://learn.adafruit.com/introducing-adafruit-stemma-qt
.. _`JTAG debugging for ESP32-S2`: https://docs.espressif.com/projects/esp-idf/en/stable/esp32s2/api-guides/jtag-debugging/index.html
