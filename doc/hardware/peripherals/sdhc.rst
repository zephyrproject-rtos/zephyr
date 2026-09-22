.. _sdhc_api:

Secure Digital (SD card) interface
###################################

Zephyr can communicate with an attached SD card either using a system's native
SD card interface or over SPI (Serial Peripheral Interface). Some devices can
also communicate with MMC (MultiMediaCard) devices.

Applications can use Zephyr's :ref:`the disk access API <disk_access_api>`
to use SD cards as storage devices, or Zephyr's SD card subsystem to
directly read from and write to a card.

SD Host Controller (SDHC)
*************************

An SD host controller (SDHC) is a device capable of sending commands to an
SD card. These commands can be sent using a system's native SD card interface,
or over SPI.

Applications should generally not use the SD host controller API directly,
instead they should use Zephyr's SD card subsystem.

Requests
========

The core of the SD host controller (SDHC) API is the :c:func:`sdhc_request` API.
Requests contain a :c:struct:`sdhc_command` command structure, and an optional
:c:struct:`sdhc_data` data structure. The caller may check the return code,
or the ``response`` field of the SD command structure to determine if the
SDHC request succeeded. The data structure allows the caller to specify a
number of blocks to transfer, and a buffer location to read or write them from.
Whether the provided buffer is used for sending or reading data depends on the
command opcode provided.

Host Controller I/O
===================

The :c:func:`sdhc_set_io` API allows the user to change I/O settings of the SD
host controller, such as clock frequency, I/O voltage, and card power. Not all
controllers will support applying all I/O settings. For example, SPI mode
controllers typically cannot toggle power to the SD card.

Related configuration options:

* :kconfig:option:`CONFIG_SDHC`

API Reference
*************

.. doxygengroup:: sdhc_interface
