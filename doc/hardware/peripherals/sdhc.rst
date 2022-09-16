.. _sdhc_api:


SDHC
####

The SDHC api offers a generic interface for interacting with an SD host
controller device. It is used by the SD subsystem, and is not intended to be
directly used by the application

Basic Operation
***************

SD Host Controller
==================

An SD host controller is a device capable of sending SD commands to an attached
SD card. These commands can be sent using the native SD protocol, or over SPI.
the SDHC api is designed to provide a generic way to send commands to and
interact with attached SD devices.

Requests
========

The core of the SDHC api is the :c:func:`sdhc_request` api. Requests contain a
:c:struct:`sdhc_command` command structure, and an optional
:c:struct:`sdhc_data` data structure. The caller may check the return code,
or the ``response`` field of the SD command structure to determine if the
SDHC request succeeded. The data structure allows the caller to specify a
number of blocks to transfer, and a buffer location to read or write them from.
Whether the provided buffer is used for sending or reading data depends on the
command opcode provided.

Host Controller I/O
===================

The :c:func:`sdhc_set_io` api allows the user to change I/O settings of the SD
host controller, such as clock frequency, I/O voltage, and card power. Not all
controllers will support applying all I/O settings. For example, SPI mode
controllers typically cannot toggle power to the SD card.

Related configuration options:

* :kconfig:option:`CONFIG_SDHC`

API Reference
*************

.. doxygengroup:: sdhc_interface
