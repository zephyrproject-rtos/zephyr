.. zephyr:board:: adafruit_qt_py_esp32s3

Overview
********

An Adafruit based Xiao compatible board based on the ESP32-S3, which is great
for IoT projects and prototyping with new sensors.

For more details see the `Adafruit QT Py ESP32S3`_ product page.

Hardware
********

This board comes in 2 variants, both based on the ESP32-S3 with WiFi and BLE
support. The default variant supporting 8MB of flash with no PSRAM, while the
``psram`` variant supporting 4MB of flash with 2MB of PSRAM. Both boards have a
USB-C port for programming and debugging and is based on a standard XIAO 14
pin pinout.

In addition to the Xiao compatible pinout, it also has a RGB NeoPixel for
status and debugging, a reset button, and a button for entering the ROM
bootloader or user input. Like many other Adafruit boards, it has a
`SparkFun Qwiic`_-compatible `STEMMA QT`_ connector for the I2C bus so you
don't even need to solder.

ESP32-S3 is a low-power MCU-based system on a chip (SoC) with integrated
2.4 GHz Wi-Fi and Bluetooth® Low Energy (Bluetooth LE). It consists of
high-performance dual-core microprocessor (Xtensa® 32-bit LX7), a low power
coprocessor, a Wi-Fi baseband, a Bluetooth LE baseband, RF module, and
numerous peripherals.

Supported Features
==================

.. zephyr:board-supported-hw::

Prerequisites
-------------

Espressif HAL requires WiFi and Bluetooth binary blobs in order work. Run the
command below to retrieve those files.

.. code-block:: console

   west blobs fetch hal_espressif

.. note::

   It is recommended running the command above after :file:`west update`.

Building & Flashing
*******************

.. zephyr:board-supported-runners::

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
   :tool: west
   :zephyr-app: samples/hello_world
   :board: adafruit_qt_py_esp32s3
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

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

.. tabs::

   .. group-tab:: QT Py ESP32S3

      .. zephyr-app-commands::
         :zephyr-app: samples/hello_world
         :board: adafruit_qt_py_esp32s3/esp32s3/procpu
         :goals: build

   .. group-tab:: QT Py ESP32S3 with PSRAM

      .. zephyr-app-commands::
         :zephyr-app: samples/hello_world
         :board: adafruit_qt_py_esp32s3@psram/esp32s3/procpu
         :goals: build

The usual ``flash`` target will work with the ``adafruit_qt_py_esp32s3`` board
configuration. Here is an example for the :zephyr:code-sample:`hello_world`
application.

.. tabs::

   .. group-tab:: QT Py ESP32S3

      .. zephyr-app-commands::
         :zephyr-app: samples/hello_world
         :board: adafruit_qt_py_esp32s3/esp32s3/procpu
         :goals: flash

   .. group-tab:: QT Py ESP32S3 with PSRAM

      .. zephyr-app-commands::
         :zephyr-app: samples/hello_world
         :board: adafruit_qt_py_esp32s3@psram/esp32s3/procpu
         :goals: flash

Open the serial monitor using the following command:

.. code-block:: shell

   west espressif monitor

After the board has automatically reset and booted, you should see the following
message in the monitor:

.. code-block:: console

   ***** Booting Zephyr OS vx.x.x-xxx-gxxxxxxxxxxxx *****
   Hello World! adafruit_qt_py_esp32s3/esp32s3/procpu

Debugging
*********

ESP32-S3 support on OpenOCD is available at `OpenOCD ESP32`_.

ESP32-S3 has a built-in JTAG circuitry and can be debugged without any
additional chip. Only an USB cable connected to the D+/D- pins is necessary.

Further documentation can be obtained from the SoC vendor
in `JTAG debugging for ESP32-S3`_.

Here is an example for building the :zephyr:code-sample:`hello_world` application.

.. tabs::

   .. group-tab:: QT Py ESP32S3

      .. zephyr-app-commands::
         :zephyr-app: samples/hello_world
         :board: adafruit_qt_py_esp32s3/esp32s3/procpu
         :goals: debug

   .. group-tab:: QT Py ESP32S3 with PSRAM

      .. zephyr-app-commands::
         :zephyr-app: samples/hello_world
         :board: adafruit_qt_py_esp32s3@psram/esp32s3/procpu
         :goals: debug

You can debug an application in the usual way. Here is an example for
the :zephyr:code-sample:`hello_world` application.

.. tabs::

   .. group-tab:: QT Py ESP32S3

      .. zephyr-app-commands::
         :zephyr-app: samples/hello_world
         :board: adafruit_qt_py_esp32s3/esp32s3/procpu
         :goals: debug

   .. group-tab:: QT Py ESP32S3 with PSRAM

      .. zephyr-app-commands::
         :zephyr-app: samples/hello_world
         :board: adafruit_qt_py_esp32s3@psram/esp32s3/procpu
         :goals: debug

References
**********

.. target-notes::

.. _`Adafruit QT Py ESP32S3`: https://www.adafruit.com/product/5426
.. _`Adafruit QT Py ESP32S3 - PSRAM`: https://www.adafruit.com/product/5700
.. _`JTAG debugging for ESP32-S3`: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/jtag-debugging/
.. _`OpenOCD ESP32`: https://github.com/espressif/openocd-esp32/releases
.. _`SparkFun Qwiic`: https://www.sparkfun.com/qwiic
.. _`STEMMA QT`: https://learn.adafruit.com/introducing-adafruit-stemma-qt
