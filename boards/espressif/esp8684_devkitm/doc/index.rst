.. zephyr:board:: esp8684_devkitm

Overview
********

The ESP8684-DevKitM is an entry-level development board based on ESP8684-MINI-1, a general-purpose
module with 1 MB/2 MB/4 MB SPI flash. This board integrates complete Wi-Fi and Bluetooth LE functions.
For more information, check `ESP8684-DevKitM User Guide`_

Hardware
********

ESP32-C2 (ESP8684 core) is a low-cost, Wi-Fi 4 & Bluetooth 5 (LE) chip. Its unique design
makes the chip smaller and yet more powerful than ESP8266. ESP32-C2 is built around a RISC-V
32-bit, single-core processor, with 272 KB of SRAM (16 KB dedicated to cache) and 576 KB of ROM.
ESP32-C2 has been designed to target simple, high-volume, and low-data-rate IoT applications,
such as smart plugs and smart light bulbs. ESP32-C2 offers easy and robust wireless connectivity,
which makes it the go-to solution for developing simple, user-friendly and reliable
smart-home devices. For more information, check `ESP8684 Datasheet`_.

Features include the following:

- 32-bit core RISC-V microcontroller with a maximum clock speed of 120 MHz
- 2 MB or 4 MB in chip (ESP8684) or in package (ESP32-C2) flash
- 272 KB of internal RAM
- 802.11b/g/n
- A Bluetooth LE subsystem that supports features of Bluetooth 5 and Bluetooth Mesh
- Various peripherals:

  - 14 programmable GPIOs
  - 3 SPI
  - 2 UART
  - 1 I2C Master
  - LED PWM controller, with up to 6 channels
  - General DMA controller (GDMA)
  - 1 12-bit SAR ADC, up to 5 channels
  - 1 temperature sensor
  - 1 54-bit general-purpose timer
  - 2 watchdog timers
  - 1 52-bit system timer

- Cryptographic hardware acceleration (RNG, ECC, RSA, SHA-2, AES)

For detailed information check `ESP8684 Technical Reference Manual`_.

Supported Features
==================

.. zephyr:board-supported-hw::

For a getting started user guide, please check `ESP8684-DevKitM User Guide`_.

System requirements
*******************

Prerequisites
=============

Espressif HAL requires WiFi and Bluetooth binary blobs in order work. Run the command
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

The board could be loaded using the single binary image, without 2nd stage bootloader.
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
   :board: esp8684_devkitm
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

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: esp8684_devkitm
   :goals: build

The usual ``flash`` target will work with the ``esp8684_devkitm`` board
configuration. Here is an example for the :zephyr:code-sample:`hello_world`
application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: esp8684_devkitm
   :goals: flash

Open the serial monitor using the following command:

.. code-block:: shell

   west espressif monitor

After the board has automatically reset and booted, you should see the following
message in the monitor:

.. code-block:: console

   ***** Booting Zephyr OS vx.x.x-xxx-gxxxxxxxxxxxx *****
   Hello World! esp8684_devkitm

Debugging
*********

As with much custom hardware, the ESP8684 modules require patches to
OpenOCD that are not upstreamed yet. Espressif maintains their own fork of
the project. The custom OpenOCD can be obtained at `OpenOCD ESP32`_.

The Zephyr SDK uses a bundled version of OpenOCD by default. You can overwrite that behavior by adding the
``-DOPENOCD=<path/to/bin/openocd> -DOPENOCD_DEFAULT_PATH=<path/to/openocd/share/openocd/scripts>``
parameter when building.

Here is an example for building the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: esp8684_devkitm
   :goals: build flash
   :gen-args: -DOPENOCD=<path/to/bin/openocd> -DOPENOCD_DEFAULT_PATH=<path/to/openocd/share/openocd/scripts>

You can debug an application in the usual way. Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: esp8684_devkitm
   :goals: debug

References
**********

.. target-notes::

.. _`ESP8684-DevKitM User Guide`: https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp8684/esp8684-devkitm-1/user_guide.html
.. _`ESP8684 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp8684_datasheet_en.pdf
.. _`ESP8684 Technical Reference Manual`: https://www.espressif.com/sites/default/files/documentation/esp8684_technical_reference_manual_en.pdf
.. _`OpenOCD ESP32`: https://github.com/espressif/openocd-esp32/releases
