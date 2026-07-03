.. _nvmem:

Non-Volatile Memory (NVMEM)
###########################

The NVMEM subsystem provides a generic interface for accessing non-volatile
memory devices. It abstracts the underlying hardware and provides a unified API
for reading and writing data.

Key Concepts
************

NVMEM Provider
==============

An NVMEM provider is a driver that exposes NVMEM cells. For example, an EEPROM
driver can be an NVMEM provider. The NVMEM provider is responsible for reading
and writing data to the underlying hardware.

NVMEM Cell
==========

An NVMEM cell is a region of non-volatile memory. It is defined in the
devicetree and has properties like offset, size, and read-only status.

NVMEM Consumer
==============

An NVMEM consumer is a driver or application that uses NVMEM cells to store or
retrieve data.

Configuration
*************

* :kconfig:option:`CONFIG_NVMEM`: Enables the NVMEM subsystem.
* :kconfig:option:`CONFIG_NVMEM_BBRAM`: Enables NVMEM support for Battery Backed RAM.
* :kconfig:option:`CONFIG_NVMEM_EEPROM`: Enables NVMEM support for EEPROM devices.
* :kconfig:option-regex:`CONFIG_NVMEM_FLASH.*`: Configure NVMEM support for flash devices.
* :kconfig:option-regex:`CONFIG_NVMEM_OTP.*`: Configure NVMEM support for OTP devices.
* :kconfig:option:`CONFIG_NVMEM_MMIO`: Enables NVMEM support for memory mapped IO regions.

Memory Mapped IO
****************

A ``fixed-layout`` provider can be marked with the ``mmio`` property to access
its cells directly through the parent controller's memory mapped IO region
rather than a device driver API. The controller's ``reg`` property is then
interpreted as a physical base address. For each read or write, the subsystem
maps the region on demand as uncached memory, copies the data, and unmaps it
again. This map-on-demand model fits the infrequent access typical of NVMEM
cells.

.. warning::

   Reads and writes are plain memory accesses, so the backing memory must
   behave like RAM: directly memory mapped and byte-writable, such as MRAM,
   FRAM or battery-backed SRAM.

   Do **not** use ``mmio`` for memory mapped flash. Flash can be read this way,
   but a write is a plain store that does not perform the erase/program
   sequence; depending on the hardware it is either ignored or raises a bus
   fault. For flash, mark the cells ``read-only`` or access them through the
   flash device API instead.

Devicetree Bindings
*******************

The NVMEM subsystem relies on devicetree bindings to define NVMEM cells.
The following is an example of how to define an NVMEM provider and cells in the
devicetree:

.. literalinclude:: devicetree_bindings.txt
   :language: dts

The reg property is an array containing:

* The offset in the memory in which we are creating the cell,
* The size of the cell, in bytes.

``#nvmem-cell-cells`` describes the number of property items in the phandle,
see :ref:`dt-bindings-cells`, typically set to zero.

A consumer can then reference the NVMEM cells like this:

.. literalinclude:: my_consumer.txt
   :language: dts


Usage Example
*************

The following is an example of how to use the NVMEM API to read data from an
NVMEM cell:

.. literalinclude:: usage_example.txt
   :language: c


API Reference
*************

.. doxygengroup:: nvmem_interface
