.. _retained_mem_api:

Retained Memory
###############

Overview
********

The retained memory driver API provides a way of reading from/writing to memory
areas whereby the contents of the memory is retained whilst the device is
powered (data may be lost in low power modes).

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_RETAINED_MEM`
* :kconfig:option:`CONFIG_RETAINED_MEM_INIT_PRIORITY`

API Reference
*************

.. doxygengroup:: retained_mem_interface
