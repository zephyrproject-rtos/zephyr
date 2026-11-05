.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

.. zephyr:board:: wio_tracker_l2

Overview
********

The Seeed Studio Wio Tracker L2 Pro is an ESP32-S3-based LoRa and GNSS tracker with
16 MB of quad SPI flash, 8 MB of octal PSRAM, an SX1262 radio, an L76K receiver,
and a 3.2-inch touchscreen. See the `Wio Tracker L2 wiki`_ for hardware details.

Speaker playback is supported; the microphone is not enabled.

Supported Features
******************

.. zephyr:board-supported-hw::

The following devicetree identifiers expose the integrated peripherals:

.. list-table::
   :header-rows: 1

   * - Identifier
     - Peripheral
   * - ``led0``, ``led1`` aliases
     - Mesh LED (GPIO43) and red user LED (GPIO expander P1.2), both active high
   * - ``sw0``, ``sw1`` aliases
     - USER/BOOT (GPIO0) and WAKE-UP (GPIO expander P0.0), both active low
   * - ``volt-sensor0`` alias
     - Battery voltage through the ADS1115 ADC and a 2:1 voltage divider
   * - ``zephyr,display`` chosen node
     - NV3031B LCD with LP5814 backlight, 320 by 240 pixels in landscape orientation
   * - ``zephyr,touch`` chosen node
     - GT911 touchscreen, with coordinates matching the display orientation
   * - ``sdhc0`` alias
     - microSD slot using 1-bit SDMMC, disk name ``SD``
   * - ``audio_codec`` node label, ``i2s-codec-tx`` alias
     - ES8311 codec and I2S interface for speaker playback
   * - ``lora0`` alias
     - SX1262 radio using the native SX126x driver
   * - ``gnss`` alias
     - L76K receiver using the generic NMEA driver on UART1

The display brightness and blanking APIs also control the backlight. Touch input
uses polling because the GPIO expander can miss short interrupt pulses. The LCD and
touchscreen share an always-enabled supply.

The GNSS driver reports navigation and satellite data but does not support changing
the fix rate or enabled constellations.

Programming
***********

.. zephyr:board-supported-runners::

The USB Type-C connector provides the console and flashing interface through the
ESP32-S3 USB Serial/JTAG controller. The ROM download bootloader requires no external
programmer or preinstalled flash bootloader. The default build uses
:kconfig:option:`CONFIG_SOC_FAMILY_ESPRESSIF_SIMPLE_BOOT` and replaces the existing
application in flash.

Build the :zephyr:code-sample:`hello_world` sample for the main CPU:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: wio_tracker_l2/esp32s3/procpu
   :goals: build

Connect the board to USB. To enter the ROM bootloader manually, hold USER/BOOT, press
and release RST, and then release USER/BOOT. Flash the application with:

.. code-block:: console

   west flash --esp-device /dev/ttyACM0 --esp-no-stub --reset-type watchdog-reset

Replace ``/dev/ttyACM0`` with the board's serial port, such as ``/dev/cu.usbmodem*`` on
macOS. If the board remains in download mode, press RST with USER/BOOT released.
Open the same serial port at 115200 baud and reset the board to see the console output.

Before building Bluetooth LE or Wi-Fi applications, fetch the Espressif binary libraries:

.. code-block:: console

   west blobs fetch hal_espressif

Peripheral Samples
==================

The following samples use the board's default peripheral configuration:

* :zephyr:code-sample:`blinky` and :zephyr:code-sample:`input-dump` for LEDs, buttons,
  and touch input.
* :zephyr:code-sample:`soc_voltage` for battery measurement and
  :zephyr:code-sample:`fs` for microSD access.
* :zephyr:code-sample:`display` and :zephyr:code-sample:`i2s_codec` for the screen
  and speaker.
* :zephyr:code-sample:`ble_peripheral_hr` and :zephyr:code-sample:`wifi-shell`
  for Bluetooth LE and Wi-Fi.
* :zephyr:code-sample:`lora-receive`, :zephyr:code-sample:`lora-send`, and
  :zephyr:code-sample:`gnss` for LoRa and GNSS.

References
**********

The `board image`_ is by Seeed Studio, licensed under `CC BY-SA 4.0`_. Its background
has been removed, and the image has been resized and converted to WebP.

.. _board image:
   https://media-cdn.seeedstudio.com/media/catalog/product/cache/bb49d3ec4ee05b6f018e93f896b8a25d/2/-/2-wio-tracke-l2-pro_1.jpg

.. _CC BY-SA 4.0:
   https://creativecommons.org/licenses/by-sa/4.0/

.. _Wio Tracker L2 wiki:
   https://wiki.seeedstudio.com/meshtastic_wio_tracker_l2_intro/
