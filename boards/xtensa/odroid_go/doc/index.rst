.. _odroid_go:

ODROID-GO
#########

Overview
********

ODROID-GO Game Kit is a "Do it yourself" ("DIY") portable game console by
HardKernel. It features a custom ESP32-WROVER with 16 MB flash and it operates
from 80 MHz - 240 MHz [1]_.

The features include the following:

- Dual core Xtensa microprocessor (LX6), running at 80 -  240MHz
- 4 MB of PSRAM
- 802.11b/g/n/e/i
- Bluetooth v4.2 BR/EDR and BLE
- 2.4 inch 320x240 TFT LCD
- Speaker
- Micro SD card slot
- Micro USB port (battery charging and USB_UART data communication
- Input Buttons (Menu, Volume, Select, Start, A, B, Direction Pad)
- Expansion port (I2C, GPIO, SPI)
- Cryptographic hardware acceleration (RNG, ECC, RSA, SHA-2, AES)

.. figure:: img/odroid_go.jpg
        :align: center
        :alt: ODROID-GO

        ODROID-Go Game Kit

External Connector
==================

+-------+------------------+-------------------------+
| PIN # | Signal Name      | ESP32-WROVER Functions  |
+=======+==================+=========================+
| 1     | GND              | GND                     |
+-------+------------------+-------------------------+
| 2     | VSPI.SCK (IO18)  | GPIO18, VSPICLK         |
+-------+------------------+-------------------------+
| 3     | IO12             | GPIO12                  |
+-------+------------------+-------------------------+
| 4     | IO15             | GPIO15, ADC2_CH3        |
+-------+------------------+-------------------------+
| 5     | IO4              | GPIO4, ADC2_CH0         |
+-------+------------------+-------------------------+
| 6     | P3V3             | 3.3 V                   |
+-------+------------------+-------------------------+
| 7     | VSPI.MISO (IO19) | GPIO19, VSPIQ           |
+-------+------------------+-------------------------+
| 8     | VSPI.MOSI (IO23) | GPIO23, VSPID           |
+-------+------------------+-------------------------+
| 9     | N.C              | N/A                     |
+-------+------------------+-------------------------+
| 10    | VBUS             | USB VBUS (5V)           |
+-------+------------------+-------------------------+

Supported Features
==================

The Zephyr odroid_go board configuration supports the following hardware
features:

+------------+------------+-------------------------------------+
| Interface  | Controller | Driver/Component                    |
+============+============+=====================================+
| UART       | on-chip    | serial port                         |
+------------+------------+-------------------------------------+
| GPIO       | on-chip    | gpio                                |
+------------+------------+-------------------------------------+
| PINMUX     | on-chip    | pinmux                              |
+------------+------------+-------------------------------------+
| I2C        | on-chip    | i2c                                 |
+------------+------------+-------------------------------------+
| SPI        | on-chip    | spi                                 |
+------------+------------+-------------------------------------+


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

ESP-IDF bootloader
==================

The board is using the ESP-IDF bootloader as the default 2nd stage bootloader.
It is build as a subproject at each application build. No further attention
is expected from the user.

MCUboot bootloader
==================

User may choose to use MCUboot bootloader instead. In that case the bootloader
must be build (and flash) at least once.

There are two options to be used when building an application:

1. Sysbuild
2. Manual build

.. note::

   User can select the MCUboot bootloader by adding the following line
   to the board default configuration file.
   ```
   CONFIG_BOOTLOADER_MCUBOOT=y
   ```

Sysbuild
========

The sysbuild makes possible to build and flash all necessary images needed to
bootstrap the board with the ESP32 SoC.

To build the sample application using sysbuild use the command:

.. zephyr-app-commands::
   :tool: west
   :app: samples/hello_world
   :board: odroid_go
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
For that reason, images can be build one at a time using traditional build.

The instructions following are relevant for both manual build and sysbuild.
The only difference is the structure of the build directory.

.. note::

   Remember that bootloader (MCUboot) needs to be flash at least once.

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: odroid_go
   :goals: build

The usual ``flash`` target will work with the ``odroid_go`` board
configuration. Here is an example for the :ref:`hello_world`
application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: odroid_go
   :goals: flash

Open the serial monitor using the following command:

.. code-block:: shell

   west espressif monitor

After the board has automatically reset and booted, you should see the following
message in the monitor:

.. code-block:: console

   ***** Booting Zephyr OS vx.x.x-xxx-gxxxxxxxxxxxx *****
   Hello World! odroid_go

Debugging
*********

As with much custom hardware, the ESP32 modules require patches to
OpenOCD that are not upstreamed yet. Espressif maintains their own fork of
the project. The custom OpenOCD can be obtained at `OpenOCD ESP32`_

The Zephyr SDK uses a bundled version of OpenOCD by default. You can overwrite that behavior by adding the
``-DOPENOCD=<path/to/bin/openocd> -DOPENOCD_DEFAULT_PATH=<path/to/openocd/share/openocd/scripts>``
parameter when building.

Here is an example for building the :ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: odroid_go
   :goals: build flash
   :gen-args: -DOPENOCD=<path/to/bin/openocd> -DOPENOCD_DEFAULT_PATH=<path/to/openocd/share/openocd/scripts>

You can debug an application in the usual way. Here is an example for the :ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: odroid_go
   :goals: debug

References
**********

.. target-notes::

.. [1] https://wiki.odroid.com/odroid_go/odroid_go
.. _`OpenOCD ESP32`: https://github.com/espressif/openocd-esp32/releases
