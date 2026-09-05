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

Devices implementing one of the memory device APIs (BBRAM, EEPROM, flash and
OTP) act as NVMEM providers as-is. Other devices can expose NVMEM cells by
implementing the dedicated NVMEM provider device API, see
:c:struct:`nvmem_driver_api`. For such providers the cell's devicetree reg
address is passed to the driver unmodified and does not have to be a byte
offset into a flat memory space; the byte offset within the cell is passed
separately.

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
* :kconfig:option:`CONFIG_NVMEM_PROVIDER`: Enabled when a native NVMEM provider driver is selected.
* :kconfig:option:`CONFIG_NVMEM_BBRAM`: Enables NVMEM support for Battery Backed RAM.
* :kconfig:option:`CONFIG_NVMEM_EEPROM`: Enables NVMEM support for EEPROM devices.
* :kconfig:option-regex:`CONFIG_NVMEM_FLASH.*`: Configure NVMEM support for flash devices.
* :kconfig:option-regex:`CONFIG_NVMEM_OTP.*`: Configure NVMEM support for OTP devices.
* :kconfig:option-regex:`CONFIG_NVMEM_PSA.*`: Configure NVMEM support for PSA Secure Storage
  entries.

PSA Secure Storage
******************

The :dtcompatible:`zephyr,nvmem-psa-its` and :dtcompatible:`zephyr,nvmem-psa-ps`
providers expose PSA Secure Storage entries (Internal Trusted Storage,
respectively Protected Storage) as NVMEM cells. Each cell maps to one storage
entry whose UID is the cell's reg address plus the provider's optional
``uid-base`` property. This allows entries that already exist, for example
ones written by another component or during provisioning, to be consumed
through the NVMEM API:

.. code-block:: dts

   / {
           psa_its: nvmem-psa-its {
                   compatible = "zephyr,nvmem-psa-its";

                   nvmem-layout {
                           compatible = "fixed-layout";
                           #address-cells = <1>;
                           #size-cells = <1>;

                           /* pre-shared key provisioned elsewhere */
                           device_psk: device-psk@2b001000 {
                                   reg = <0x2b001000 16>;
                                   read-only;
                                   #nvmem-cell-cells = <0>;
                           };
                   };
           };
   };

The PSA Secure Storage APIs are provided either by TF-M
(:kconfig:option:`CONFIG_BUILD_WITH_TFM`) or by the
:ref:`secure storage subsystem <secure_storage>`. Refer to their documentation
for the properties and limits of the storage itself, such as entry sizes,
valid UIDs and which entries are reachable.

A cell is a window over the start of its entry. Compared to memory-backed
cells:

* Reading a cell whose entry does not exist fails with ``-ENOENT``, and
  reading beyond the stored entry size fails.
* Writing part of a cell read-modify-writes the whole backing entry, which
  must fit :kconfig:option:`CONFIG_NVMEM_PSA_WRITE_BUF_SIZE`. A write
  starting at offset 0 to a non-existing entry creates it.
* Entries are never created with ``PSA_STORAGE_FLAG_WRITE_ONCE``; writes
  refused by the PSA implementation fail with ``-EROFS``.

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
