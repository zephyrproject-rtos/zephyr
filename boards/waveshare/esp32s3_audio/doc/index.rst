.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

.. zephyr:board:: esp32s3_audio

Overview
********

The `ESP32-S3-AUDIO-Board`_ from Waveshare is an ESP32-S3R8 development board with
16 MB flash, 8 MB octal PSRAM, two microphones, a speaker connector, seven RGB LEDs,
a real-time clock, and a microSD slot. It also exposes display and camera connectors.

This board definition uses the wiring in the `version 1.1 schematic`_.

Hardware
********

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

Supported Features
==================

.. zephyr:board-supported-hw::

Connections
===========

.. list-table:: Onboard connections
   :header-rows: 1
   :widths: 35 65

   * - Function
     - Connection
   * - USB Serial/JTAG console
     - GPIO19 (D-), GPIO20 (D+)
   * - UART0
     - GPIO43 (TX), GPIO44 (RX)
   * - I2C0
     - GPIO10 (SCL), GPIO11 (SDA)
   * - TCA9555 GPIO expander
     - I2C address 0x20
   * - PCF85063A RTC
     - I2C address 0x51
   * - BOOT button (``sw0``)
     - GPIO0, active low
   * - KEY1, KEY2, KEY3 (``sw1``, ``sw2``, ``sw3``)
     - Expander pins 9, 10, 11, active low
   * - WS2812B RGB LED ring (``led-strip``)
     - GPIO38, seven LEDs driven by RMT channel 0
   * - microSD (one-bit SDMMC)
     - GPIO40 (CLK), GPIO41 (D0), GPIO42 (CMD)
   * - SPI display connector
     - GPIO3 (CS), GPIO4 (CLK), GPIO8 (MISO), GPIO9 (MOSI)
   * - I2S0
     - GPIO12 (MCLK), GPIO13 (BCLK), GPIO14 (LRCLK), GPIO15 (RX), GPIO16 (TX)

The TCA9555 uses the PCA9555 driver through its register-compatible fallback.
The expander interrupt is not connected on the stock board (R15 is unpopulated),
so KEY1 through KEY3 use polling. The RTC interrupt is connected to expander pin 4;
RTC alarm interrupts are not configured.

The SD card slot is wired for one-bit operation. DAT1 and DAT2 are not connected to
the ESP32-S3, and DAT3 is connected to expander pin 3.

RTC and microSD
===============

Enable :kconfig:option:`CONFIG_RTC` to access the PCF85063A through the ``rtc`` alias.
The driver supports setting and reading time. With :kconfig:option:`CONFIG_RTC_ALARM`,
applications can configure an alarm and poll :c:func:`rtc_alarm_is_pending` without
an interrupt connection. For interactive time access, enable
:kconfig:option:`CONFIG_SHELL` and :kconfig:option:`CONFIG_RTC_SHELL`.

The microSD slot supports reads and writes through the disk named ``SD``. Applications
using FatFs can mount it at ``/SD:`` with the following configuration:

.. code-block:: cfg

   CONFIG_DISK_ACCESS=y
   CONFIG_FILE_SYSTEM=y
   CONFIG_FAT_FILESYSTEM_ELM=y
   CONFIG_FS_FATFS_MOUNT_MKFS=n

Enable :kconfig:option:`CONFIG_FS_FATFS_EXFAT` for an exFAT-formatted card. Set
``FS_MOUNT_FLAG_NO_FORMAT`` in the application's mount flags to preserve the existing
filesystem if mounting fails. See :zephyr:code-sample:`fs` for filesystem API usage.

Audio and expansion
===================

The I2S0 pins are configured for the onboard ES8311 codec and ES7210 microphone ADC.
These devices do not yet have Zephyr drivers, so speaker playback and microphone capture
are not supported by this board definition. Their I2C addresses are 0x18 and 0x40,
respectively. The speaker amplifier enable signal is on expander pin 8.

An attached display or camera requires an application overlay. The display connector
uses GPIO7 for D/C, GPIO5 for backlight control, and expander pin 0 for reset.
Expander pin 6 selects the camera clock signals. A high level routes camera XCLK and PCLK
to GPIO43 and GPIO44, respectively; disable UART0 when using this route. A low level routes
them to GPIO20 and GPIO19; disable USB Serial/JTAG and move the console when using that route.
Configure the switch before enabling camera capture.

Battery voltage measurement requires soldering the optional resistor shown in the
schematic and conflicts with the camera HREF signal on GPIO1. It is not enabled.

Overlay templates
-----------------

The board's ``support/display.overlay`` and ``support/camera.overlay`` files provide
starting points for application overlays. Copy the appropriate file to the application's
``app.overlay`` and fill in the attached module's binding and required properties.
The templates do not instantiate a display or camera sensor, and camera capture remains
disabled. Neither connector has been tested with an attached module.

For a display, add its controller node under ``spi2`` or a MIPI DBI bus as required by
the panel binding. Configure reset, D/C, and backlight control for the panel, and set
``zephyr,display`` in ``/chosen`` to the display node.

For a camera, add its sensor node on ``i2c0``, the parallel interface pinctrl, and the
sensor source and endpoint connections for ``lcd_cam_dvp``. Configure the camera switch
and resolve the USB and UART pin conflicts before enabling capture. Set ``zephyr,camera``
in ``/chosen`` to ``lcd_cam_dvp`` when the configuration is complete.

System Requirements
*******************

.. include:: ../../../espressif/common/system-requirements.rst
   :start-after: espressif-system-requirements

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Connect the USB Type-C port to the host. The default console uses the built-in USB
Serial/JTAG controller. UART0 is available on GPIO43 and GPIO44 with an external adapter.

Build and flash the :zephyr:code-sample:`hello_world` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: esp32s3_audio/esp32s3/procpu
   :goals: build flash

If the board does not enter download mode automatically, hold BOOT, press and release
RESET, then release BOOT before flashing. If it remains in download mode after flashing,
press RESET or use ``west flash --reset-type watchdog-reset``.

The :zephyr:code-sample:`led-strip` sample drives the seven RGB LEDs, and
:zephyr:code-sample:`input-dump` reports events from BOOT and the three user buttons.

MCUboot bootloader
==================

Simple boot is the default. To build MCUboot together with a signed application, use sysbuild:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: esp32s3_audio/esp32s3/procpu
   :west-args: --sysbuild
   :gen-args: -DSB_CONFIG_BOOTLOADER_MCUBOOT=y
   :goals: build

Flash both images with ``west flash --reset-type watchdog-reset``. The 16 MB flash layout
reserves 64 KB at offset 0 for MCUboot, with primary and secondary application slots at
offsets 0x20000 and 0x5f0000. Each slot is 5952 KB. Separate partitions are reserved for
the APPCPU and low-power core images.

Sysbuild signs the application and embeds the matching public key in MCUboot. The default
is MCUboot's development key; configure ``SB_CONFIG_BOOT_SIGNATURE_KEY_FILE`` for an
application-specific key.
To validate the primary image on every boot, enable ``CONFIG_BOOT_VALIDATE_SLOT0`` in
the application's ``sysbuild/mcuboot.conf``. The ESP32-S3 MCUboot defaults disable this check.

AMP applications
================

The ``esp32s3_audio/esp32s3/appcpu`` target is for a secondary image started by
PROCPU in an AMP application. It has no standalone console. See the
:zephyr:code-sample:`ipm-esp32` sample for an application using both cores.

.. include:: ../../../espressif/common/board-variants.rst
   :start-after: espressif-board-variants

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

References
**********

.. _ESP32-S3-AUDIO-Board: https://docs.waveshare.com/ESP32-S3-AUDIO-Board
.. _version 1.1 schematic: https://files.waveshare.com/wiki/ESP32-S3-AUDIO-Board/ESP32-S3-AUDIO-Board_1.1.pdf
