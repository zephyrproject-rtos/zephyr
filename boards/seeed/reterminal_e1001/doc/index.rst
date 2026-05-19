.. zephyr:board:: reterminal_e1001

Overview
********

Seeed Studio reTerminal E1001 is a board with a 7.5 inch monochrome ePaper display (800×480)
based on the Espressif ESP32-S3R8 WiFi/Bluetooth dual-mode chip.

For more details see the `Seeed Studio reTerminal E1001 wiki`_ page.

Hardware
********

Board Features
==============

For more details about the board, see the `reTerminal E1001 board schematics`_.

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

.. _`Seeed Studio reTerminal E1001 wiki`:
   https://wiki.seeedstudio.com/getting_started_with_reterminal_e1001/

.. _`reTerminal E1001 board schematics`:
   https://files.seeedstudio.com/wiki/reterminal_e10xx/res/202004307_reTerminal_E1001_V1_2_SCH_251120.pdf
