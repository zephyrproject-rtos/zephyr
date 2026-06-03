.. _memc_api:

Memory Controller (MEMC)
########################

Overview
********

The MEMC API provides a generic interface for reading and writing external
memory devices (such as PSRAM) that are accessed through a bus controller
rather than being directly memory-mapped.

Drivers implementing this API translate :c:func:`memc_read` and
:c:func:`memc_write` calls into the appropriate bus transactions (e.g., MSPI
commands) for the underlying hardware.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_MEMC`
* :kconfig:option:`CONFIG_MEMC_INIT_PRIORITY`

API Reference
*************

.. doxygengroup:: memc_interface
