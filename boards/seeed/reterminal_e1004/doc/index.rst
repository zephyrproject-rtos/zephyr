.. zephyr:board:: reterminal_e1004

Overview
********

Seeed Studio reTerminal E1004 is a board with a 13.3 inch full color ePaper display with
a built-in battery based on the Espressif ESP32-S3R8 WiFi/Bluetooth dual-mode chip.

For more details see the `Seeed Studio reTerminal E1004 wiki`_ page.

Hardware
********

Board Features
==============

The board includes the following features:

- 32MB of flash
- 8MB of PSRAM
- 1200×1600 13.3 inch E Ink Spectra 6 (E6) full-color (6 colors) ePaper display
- Micro SD slot
- 5000 mAh built-in battery + Silergy SY6974B switching battery charger
- Temperature and humidity sensor
- RTC
- 3x capacitive touch buttons
- Green status LED
- Buzzer

.. note::

   The ePaper display is not yet supported.

For more details about the board, see the `reTerminal E1004 board schematics`_.

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

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

References
**********

.. target-notes::

.. _`Seeed Studio reTerminal E1004 wiki`:
   https://wiki.seeedstudio.com/getting_started_with_reterminal_e1004/

.. _`reTerminal E1004 board schematics`:
   https://files.seeedstudio.com/wiki/reterminal_e10xx/res/202004523_reTerminal%20E1004_V1.0_SCH_260105.pdf
