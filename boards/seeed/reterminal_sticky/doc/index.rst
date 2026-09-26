.. zephyr:board:: reterminal_sticky

Overview
********

Seeed Studio reTerminal Sticky is a magnetic 3.97 inch ePaper display with capacitive touch and a
built-in battery based on the Espressif ESP32-S3R8 WiFi/Bluetooth dual-mode chip.

For more details see the `Seeed Studio reTerminal Sticky documentation`_.

Hardware
********

Board Features
==============

The board includes the following features:

- 32MB of flash
- 8MB of PSRAM
- 480x800 3.97 inch monochrome ePaper display (Solomon SSD1677 controller)
- Capacitive touch panel (Goodix GT911)
- Micro SD slot
- 750 mAh built-in battery + TI BQ25616 battery charger and BQ27220 fuel gauge
- Temperature and humidity sensor (Sensirion SHT40)
- 6-axis IMU (ST LSM6DS3TR-C)
- RTC (NXP PCF8563)
- PDM microphone
- 3x buttons
- Buzzer
- USB Type-C connector with CH343P USB-to-UART converter

For more details about the board, see the `reTerminal Sticky board schematics`_.

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

Supported Features
==================

.. zephyr:board-supported-hw::

Power Latch
===========

When running from the battery, the power button only switches the system on for as long as it is
held. The board keeps itself powered through a latch that samples ``PWR_HOLD`` (GPIO45) on a
rising edge of ``PWR_LOCK`` (GPIO46). Both lines are set high by GPIO hogs in the board devicetree,
so the system stays on once the GPIO hogs have been applied during boot. Keep the power button
pressed until then.

To switch the system off while running from the battery, drive ``PWR_HOLD`` low and generate a
rising edge on ``PWR_LOCK``. When USB power is present, the system stays powered regardless of the
latch state.

By default, the board does this when the power button is held down for 1.5 seconds, and the system
switches off once the button is released. The power button reports ``INPUT_KEY_0`` events, and the
``power_longpress`` devicetree node reports a long press of it as an ``INPUT_KEY_POWER`` event. The
``long-delay-ms`` property of that node sets the hold time. Disable
:kconfig:option:`CONFIG_RETERMINAL_STICKY_POWER_OFF_KEY` to handle the long press in the
application instead. This relies on the input subsystem, which the board enables by default.

System Requirements
*******************

.. include:: ../../../espressif/common/system-requirements.rst
   :start-after: espressif-system-requirements

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

.. include:: ../../../espressif/common/building-flashing.rst
   :start-after: espressif-building-flashing

The USB Type-C connector is wired to a CH343P USB-to-UART converter, whose DTR and RTS lines drive
the ESP32-S3 reset and boot strapping pins. The ESP32-S3 built-in USB Serial/JTAG controller is not
connected, so JTAG debugging is not available.

Backup the original firmware
============================

The following command can be used to backup the original firmware:

.. code-block:: shell

   esptool -c esp32s3 -p /dev/ttyACM0 read-flash 0x0 0x2000000 fw-backup-32MB.bin

References
**********

.. target-notes::

.. _`Seeed Studio reTerminal Sticky documentation`:
   https://www.seeedstudio.com/sticky/docs/en/device-guide/hardware-overview/

.. _`reTerminal Sticky board schematics`:
   https://files.seeedstudio.com/wiki/reterminal_sticky/res/reTerminal_Sticky_Schematic_diagram_260609.pdf
