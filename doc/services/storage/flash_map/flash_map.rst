.. _flash_map_api:

Flash map
#########

The ``<zephyr/storage/flash_map.h>`` API allows accessing information about device
flash partitions via :c:struct:`flash_area` structures.

Each :c:struct:`flash_area` describes a flash partition. The API provides access
to a "flash map", which contains predefined flash areas accessible via globally
unique ID numbers. The map is created from "fixed-partition" compatible entries
in DTS file. Users may also create :c:struct:`flash_area` objects at runtime
for application-specific purposes.

This documentation uses "flash area" when referencing single "fixed-partition"
entities.

The :c:struct:`flash_area` contains a pointer to a :c:struct:`device`,
which can be used to access the flash device an area is placed on directly
with the :ref:`flash API <flash_api>`.
Each flash area is characterized by a device it is placed on, offset
from the beginning of the device and size on the device.
An additional identifier parameter is used by the :c:func:`flash_area_open`
function to find flash area in flash map.

The flash_map.h API provides functions for operating on a :c:struct:`flash_area`.
The main examples are :c:func:`flash_area_read` and :c:func:`flash_area_write`.
These functions are basically wrappers around the flash API with additional
offset and size checks, to limit flash operations to a predefined area.

Most ``<zephyr/storage/flash_map.h>`` API functions require a :c:struct:`flash_area` object pointer
characterizing the flash area they will be working on. There are two possible
methods to obtain such a pointer:

 * obtain it using :c:func:`flash_area_open`;

 * defining a :c:struct:`flash_area` type object, which requires providing
   a valid :c:struct:`device` object pointer with offset and size of the area
   within the flash device.

:c:func:`flash_area_open` uses numeric identifiers to search flash map for
:c:struct:`flash_area` objects and returns, if found, a pointer to an object
representing area with given ID.
The ID number for a flash area can be obtained from a fixed-partition
DTS node label using :c:macro:`FIXED_PARTITION_ID()`; these labels are obtained
from the devicetree as described below.

Relationship with Devicetree
****************************

The flash_map.h API uses data generated from the :ref:`devicetree_api`, in
particular its :ref:`devicetree-flash-api`. Zephyr additionally has some
partitioning conventions used for :ref:`dfu` via the MCUboot bootloader, as
well as defining partitions usable by :ref:`file systems <file_system_api>` or
other nonvolatile :ref:`storage <storage_reference>`.

Here is an example devicetree fragment which uses fixed flash partitions for
both MCUboot and a storage partition. Some details were left out for clarity.

.. literalinclude:: example_fragment.dts
   :language: DTS
   :start-after: start-after-here

Partition offset shall be expressed in relation to the flash memory beginning
address, to which the partition belongs to.

The ``boot_partition``, ``slot0_partition``, ``slot1_partition``, and
``scratch_partition`` node labels are defined for MCUboot, though not all MCUboot
configurations require all of them to be defined. See the `MCUboot
documentation`_ for more details.

The ``storage_partition`` node is defined for use by a file system or other
nonvolatile storage API.

.. _MCUboot documentation: https://docs.mcuboot.com

Numeric flash area ID is obtained by passing DTS node label to
:c:macro:`FIXED_PARTITION_ID()`; for example to obtain ID number
for ``slot0_partition``, user would invoke ``FIXED_PARTITION_ID(slot0_partition)``.

All :code:`FIXED_PARTITION_*` macros take DTS node labels as partition
identifiers.

Users do not have to obtain a :c:struct:`flash_area` object pointer
using :c:func:`flash_map_open` to get information on flash area size, offset
or device, if such area is defined in DTS file. Knowing the DTS node label
of an area, users may use :c:macro:`FIXED_PARTITION_OFFSET()`,
:c:macro:`FIXED_PARTITION_SIZE()` or :c:macro:`FIXED_PARTITION_DEVICE()`
respectively to obtain such information directly from DTS node definition.
For example to obtain offset of ``storage_partition`` it is enough to
invoke ``FIXED_PARTITION_OFFSET(storage_partition)``.

Below example shows how to obtain a :c:struct:`flash_area` object pointer
using :c:func:`flash_area_open` and DTS node label:

.. code-block:: c

   const struct flash_area *my_area;
   int err = flash_area_open(FIXED_PARTITION_ID(slot0_partition), &my_area);

   if (err != 0) {
   	handle_the_error(err);
   } else {
   	flash_area_read(my_area, ...);
   }

API Reference
*************

.. doxygengroup:: flash_area_api
