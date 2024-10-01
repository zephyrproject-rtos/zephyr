.. _esp32c3_rust:

ESP32-C3-DevKit-RUST
####################

Overview
********

ESP32-C3-DevKit-RUST is based on the ESP32-C3, a single-core Wi-Fi and Bluetooth 5 (LE) microcontroller SoC,
based on the open-source RISC-V architecture. This special board also includes the ESP32-C3-MINI-1 module,
a 6DoF IMU, a temperature and humidity sensor, a Li-Ion battery charger, and a Type-C USB. The board is designed
to be easily used in training sessions, demonstrating its capabilities with all the board peripherals.
For more information, check `ESP32-C3-DevKit-RUST`_.

Hardware
********

SoC Features:

- IEEE 802.11 b/g/n-compliant
- Bluetooth 5, Bluetooth mesh
- 32-bit RISC-V single-core processor, up to 160MHz
- 384 KB ROM
- 400 KB SRAM (16 KB for cache)
- 8 KB SRAM in RTC
- 22 x programmable GPIOs
- 3 x SPI
- 2 x UART
- 1 x I2C
- 1 x I2S
- 2 x 54-bit general-purpose timers
- 3 x watchdog timers
- 1 x 52-bit system timer
- Remote Control Peripheral (RMT)
- LED PWM controller (LEDC)
- Full-speed USB Serial/JTAG controller
- General DMA controller (GDMA)
- 1 x TWAI®
- 2 x 12-bit SAR ADCs, up to 6 channels
- 1 x temperature sensor

For more information, check the datasheet at `ESP32-C3 Datasheet`_ or the technical reference
manual at `ESP32-C3 Technical Reference Manual`_.

Supported Features
==================

Current Zephyr's ESP32-C3-DevKit-RUST board supports the following features:

+------------+------------+-------------------------------------+
| Interface  | Controller | Driver/Component                    |
+============+============+=====================================+
| UART       | on-chip    | serial port                         |
+------------+------------+-------------------------------------+
| GPIO       | on-chip    | gpio                                |
+------------+------------+-------------------------------------+
| PINMUX     | on-chip    | pinmux                              |
+------------+------------+-------------------------------------+
| USB-JTAG   | on-chip    | hardware interface                  |
+------------+------------+-------------------------------------+
| SPI Master | on-chip    | spi                                 |
+------------+------------+-------------------------------------+
| Timers     | on-chip    | counter                             |
+------------+------------+-------------------------------------+
| Watchdog   | on-chip    | watchdog                            |
+------------+------------+-------------------------------------+
| TRNG       | on-chip    | entropy                             |
+------------+------------+-------------------------------------+
| LEDC       | on-chip    | pwm                                 |
+------------+------------+-------------------------------------+
| SPI DMA    | on-chip    | spi                                 |
+------------+------------+-------------------------------------+
| TWAI       | on-chip    | can                                 |
+------------+------------+-------------------------------------+
| USB-CDC    | on-chip    | serial                              |
+------------+------------+-------------------------------------+
| ADC        | on-chip    | adc                                 |
+------------+------------+-------------------------------------+
| Wi-Fi      | on-chip    |                                     |
+------------+------------+-------------------------------------+
| Bluetooth  | on-chip    |                                     |
+------------+------------+-------------------------------------+

I2C Peripherals
===============

This board includes the following peripherals over the I2C bus:

+---------------------------+--------------+---------+
| Peripheral                | Part number  | Address |
+===========================+==============+=========+
| IMU                       | ICM-42670-P  |  0x68   |
+---------------------------+--------------+---------+
| Temperature and Humidity  | SHTC3        |  0x70   |
+---------------------------+--------------+---------+

I2C Bus Connection
==================

+---------+--------+
| Signal  | GPIO   |
+=========+========+
| SDA     | GPIO10 |
+---------+--------+
| SCL     | GPIO8  |
+---------+--------+

I/Os
====

The following devices are connected through GPIO:

+--------------+--------+
| I/O Devices  | GPIO   |
+==============+========+
| WS2812 LED   | GPIO2  |
+--------------+--------+
| LED          | GPIO7  |
+--------------+--------+
| Button/Boot  | GPIO9  |
+--------------+--------+

Power
=====

* USB type-C (*no PD compatibility*).
* Li-Ion battery charger.

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
   :board: esp32c3_rust
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
   :board: esp32c3_rust
   :goals: build

The usual ``flash`` target will work with the ``esp32c3_rust`` board
configuration. Here is an example for the :zephyr:code-sample:`hello_world`
application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: esp32c3_rust
   :goals: flash

Open the serial monitor using the following command:

.. code-block:: shell

   west espressif monitor

After the board has automatically reset and booted, you should see the following
message in the monitor:

.. code-block:: console

   ***** Booting Zephyr OS vx.x.x-xxx-gxxxxxxxxxxxx *****
   Hello World! esp32c3_rust

Debugging
*********

As with much custom hardware, the ESP32-C3 modules require patches to
OpenOCD that are not upstreamed yet. Espressif maintains their own fork of
the project. The custom OpenOCD can be obtained at `OpenOCD ESP32`_.

The Zephyr SDK uses a bundled version of OpenOCD by default. You can overwrite that behavior by adding the
``-DOPENOCD=<path/to/bin/openocd> -DOPENOCD_DEFAULT_PATH=<path/to/openocd/share/openocd/scripts>``
parameter when building.

Here is an example for building the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: esp32c3_rust
   :goals: build flash
   :gen-args: -DOPENOCD=<path/to/bin/openocd> -DOPENOCD_DEFAULT_PATH=<path/to/openocd/share/openocd/scripts>

You can debug an application in the usual way. Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: esp32c3_rust
   :goals: debug

References
**********

.. _`ESP32-C3-DevKit-RUST`: https://github.com/esp-rs/esp-rust-board/tree/v1.2
.. _`ESP32-C3 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf
.. _`ESP32-C3 Technical Reference Manual`: https://espressif.com/sites/default/files/documentation/esp32-c3_technical_reference_manual_en.pdf
.. _`OpenOCD ESP32`: https://github.com/espressif/openocd-esp32/releases
