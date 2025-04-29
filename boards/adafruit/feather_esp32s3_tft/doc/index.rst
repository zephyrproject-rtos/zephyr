.. zephyr:board:: adafruit_feather_esp32s3_tft

Overview
********

The Adafruit Feather ESP32-S3 TFT is an ESP32-S3 development board in the
Feather standard layout, sharing peripheral placement with other devices labeled
as Feathers or FeatherWings. The board is equipped with an ESP32-S3 mini module,
a LiPo battery charger, a fuel gauge, a USB-C and Qwiic/STEMMA-QT connector.
Compared to the base model, this TFT variant additionally comes with a 240x135
pixel IPS TFT color display. For more information, check
`Adafruit Feather ESP32-S3 TFT`_.

Hardware
********

- ESP32-S3 mini module, featuring the dual core 32-bit Xtensa Microprocessor
  (Tensilica LX7), running at up to 240MHz
- 512KB SRAM and either 8MB flash or 4MB flash + 2MB PSRAM, depending on the
  module variant
- USB-C directly connected to the ESP32-S3 for USB/UART and JTAG debugging
- LiPo connector and built-in battery charging when powered via USB-C
- MAX17048 fuel gauge for battery voltage and state-of-charge reporting
- Charging indicator LED, user LED, reset and boot buttons
- Built-in NeoPixel indicator RGB LED
- STEMMA QT connector for I2C devices, with switchable power for low-power mode
- 240x135 pixel IPS TFT color display with 1.14" diagonal and ST7789 chipset

Asymmetric Multiprocessing (AMP)
================================

The ESP32-S3 SoC allows 2 different applications to be executed in asymmetric
multiprocessing. Due to its dual-core architecture, each core can be enabled to
execute customized tasks in stand-alone mode and/or exchanging data over OpenAMP
framework. See :zephyr:code-sample-category:`ipc` folder as code reference.

For more information, check the datasheet at `ESP32-S3 Datasheet`_.

Supported Features
==================

The current ``adafruit_feather_esp32s3_tft`` board supports the following
hardware features:

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
| TWAI/CAN   | on-chip    | can                                 |
+------------+------------+-------------------------------------+
| ADC        | on-chip    | adc                                 |
+------------+------------+-------------------------------------+
| Timers     | on-chip    | counter                             |
+------------+------------+-------------------------------------+
| Watchdog   | on-chip    | watchdog                            |
+------------+------------+-------------------------------------+
| TRNG       | on-chip    | entropy                             |
+------------+------------+-------------------------------------+
| LEDC       | on-chip    | pwm                                 |
+------------+------------+-------------------------------------+
| MCPWM      | on-chip    | pwm                                 |
+------------+------------+-------------------------------------+
| PCNT       | on-chip    | qdec                                |
+------------+------------+-------------------------------------+
| GDMA       | on-chip    | dma                                 |
+------------+------------+-------------------------------------+
| USB-CDC    | on-chip    | serial                              |
+------------+------------+-------------------------------------+
| Wi-Fi      | on-chip    |                                     |
+------------+------------+-------------------------------------+
| Bluetooth  | on-chip    |                                     |
+------------+------------+-------------------------------------+

Connections and IOs
===================

The `Adafruit Feather ESP32-S3 TFT User Guide`_ has detailed information about
the board including `pinouts`_ and the `schematic`_.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Prerequisites
=============

Espressif HAL requires WiFi and Bluetooth binary blobs in order work. Run the
command below to retrieve those files.

.. code-block:: console

   west blobs fetch hal_espressif

.. note::

   It is recommended running the command above after :file:`west update`.

Building & Flashing
===================

Simple boot
-----------

The board could be loaded using the single binary image, without 2nd stage
bootloader. It is the default option when building the application without
additional configuration.

.. note::

   Simple boot does not provide any security features nor OTA updates.

MCUboot bootloader
------------------

User may choose to use MCUboot bootloader instead. In that case the bootloader
must be build (and flash) at least once.

There are two options to be used when building an application:

1. Sysbuild
2. Manual build

.. note::

   User can select the MCUboot bootloader by adding the following line
   to the board default configuration file.

   .. code:: cfg

      CONFIG_BOOTLOADER_MCUBOOT=y

Sysbuild
--------

The sysbuild makes possible to build and flash all necessary images needed to
bootstrap the board with the ESP32-S3 SoC.

To build the sample application using sysbuild use the command:

.. zephyr-app-commands::
   :tool: west
   :zephyr-app: samples/hello_world
   :board: adafruit_feather_esp32s3_tft/esp32s3/procpu
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

   With ``--sysbuild`` option the bootloader will be re-build and re-flash
   every time the pristine build is used.

For more information about the system build please read the :ref:`sysbuild`
documentation.

Manual build
------------

During the development cycle, it is intended to build & flash as quickly
possible. For that reason, images can be build one at a time using traditional
build.

The instructions following are relevant for both manual build and sysbuild.
The only difference is the structure of the build directory.

.. note::

   Remember that bootloader (MCUboot) needs to be flash at least once.

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: adafruit_feather_esp32s3_tft/esp32s3/procpu
   :goals: build

The usual ``flash`` target will work with the ``adafruit_feather_esp32s3_tft``
board. Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: adafruit_feather_esp32s3_tft/esp32s3/procpu
   :goals: flash

Open the serial monitor using the following command:

.. code-block:: shell

   west espressif monitor

After the board has automatically reset and booted, you should see the following
message in the monitor:

.. code-block:: console

   ***** Booting Zephyr OS vx.x.x-xxx-gxxxxxxxxxxxx *****
   Hello World! adafruit_feather_esp32s3_tft

Debugging
=========

ESP32-S3 support on OpenOCD is available upstream as of version 0.12.0. Download
and install OpenOCD from `OpenOCD`_.

ESP32-S3 has a built-in JTAG circuitry and can be debugged without any
additional chip. Only an USB cable connected to the D+/D- pins is necessary.

Further documentation can be obtained from the SoC vendor in `JTAG debugging
for ESP32-S3`_.

Here is an example for building the :zephyr:code-sample:`hello_world`
application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: adafruit_feather_esp32s3_tft/esp32s3/procpu
   :goals: build flash

You can debug an application in the usual way. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: adafruit_feather_esp32s3_tft/esp32s3/procpu
   :goals: debug

References
**********

.. target-notes::

.. _`Adafruit Feather ESP32-S3 TFT`:
   https://www.adafruit.com/product/5483

.. _`OpenOCD`:
   https://github.com/openocd-org/openocd

.. _`JTAG debugging for ESP32-S3`:
   https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/jtag-debugging/

.. _Adafruit Feather ESP32-S3 TFT User Guide:
   https://learn.adafruit.com/adafruit-esp32-s3-tft-feather

.. _pinouts:
   https://learn.adafruit.com/adafruit-esp32-s3-tft-feather/pinouts

.. _schematic:
   https://learn.adafruit.com/adafruit-esp32-s3-tft-feather/downloads

.. _ESP32-S3 Datasheet:
   https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf

.. _ESP32 Technical Reference Manual:
   https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf
