.. _eeprom_api:

Electrically Erasable Programmable Read-Only Memory (EEPROM)
############################################################

Overview
********

The EEPROM API provides read and write access to Electrically Erasable
Programmable Read-Only Memory (EEPROM) devices.

EEPROMs have an erase block size of 1 byte, a long lifetime, and allow
overwriting data on byte-by-byte access.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_EEPROM`

API Reference
*************

.. doxygengroup:: eeprom_interface
