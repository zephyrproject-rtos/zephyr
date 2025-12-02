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
* :kconfig:option:`CONFIG_NVMEM_EEPROM`: Enables NVMEM support for EEPROM devices.

Devicetree Bindings
*******************

The NVMEM subsystem relies on devicetree bindings to define NVMEM cells.
The following is an example of how to define an NVMEM provider and cells in the
devicetree:

.. literalinclude:: devicetree_bindings.txt
   :language: dts


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
