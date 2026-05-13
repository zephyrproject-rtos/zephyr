.. zephyr:board:: reterminal_e1003

Overview
********

Seeed Studio reTerminal E1003 is a board with a 10.3 inch monochrome ePaper display with 16-level
grayscale and a built-in battery based on the Espressif ESP32-S3R8 WiFi/Bluetooth dual-mode chip.

For more details see the `Seeed Studio reTerminal E1003 wiki`_ page.

Hardware
********

Board Features
==============

The board includes the following features:

- 32MB of flash
- 8MB of PSRAM
- 1404x1872 10.3 inch monochrome ePaper display with 16-level grayscale
- Micro SD slot
- 3000 mAh built-in battery + Silergy SY6974B switching battery charger
- Temperature and humidity sensor
- RTC
- 3x buttons
- Green status LED
- Buzzer

The ePaper display is not currently enabled in Zephyr because it requires display driver support.

For more details about the board, see the `reTerminal E1003 board schematics`_.

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

Supported Features
==================

.. zephyr:board-supported-hw::

System Requirements
*******************

.. include:: ../../../espressif/common/system-requirements.rst
   :start-after: espressif-system-requirements

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

.. include:: ../../../espressif/common/building-flashing.rst
   :start-after: espressif-building-flashing

Backup the original firmware
============================

The following command can be used to backup the original firmware:

.. code-block:: shell

   esptool -c esp32s3 -p /dev/ttyUSB0 read-flash 0x0 0x2000000 fw-backup-32MB.bin

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

References
**********

.. target-notes::

.. _`Seeed Studio reTerminal E1003 wiki`:
   https://wiki.seeedstudio.com/getting_started_with_reterminal_e1003/

.. _`reTerminal E1003 board schematics`:
   https://files.seeedstudio.com/wiki/reterminal_e10xx/img/e1003/202004522_reTerminal_E1003_V1.0_SCH_251231.pdf
