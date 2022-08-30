.. _flash_map_api:

Flash map
#########

The ``<storage/flash_map.h>`` API allows accessing information about device
flash partitions via :c:struct:`flash_area` structures.

Each ``struct flash_area`` describes a flash partition. The API provides access
to a "flash map", which contains predefined flash areas accessible via globally
unique ID numbers. You can also create ``flash_area`` structures at runtime for
application-specific purposes.

The ``flash_area`` structure contains the name of the flash device the
partition is part of; this name can be passed to :c:func:`device_get_binding`
to get the corresponding :c:struct:`device` structure which can be read and
written to using the :ref:`flash API <flash_api>`. The ``flash_area`` also
contains the start offset and size of the partition within the flash memory the
device represents.

The flash_map.h API provides functions for operating on a ``flash_area``. The
main examples are :c:func:`flash_area_read` and :c:func:`flash_area_write`.
These functions are basically wrappers around the flash API with input
parameter range checks. Not all flash APIs have flash_map.h wrappers, but
:c:func:`flash_area_get_device` allows easily retrieving the ``struct device``
from a ``struct flash_area``.

Use :c:func:`flash_area_open()` to access a ``struct flash_area``. This
function takes a flash area ID number and returns a pointer to the flash area
structure. The ID number for a flash area can be obtained from a human-readable
"label" using :c:macro:`FLASH_AREA_ID`; these labels are obtained from the
devicetree as described below.

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

Rule for offsets is that each partition offset shall be expressed in relation to
the flash memory beginning address to which the partition belong.

The ``boot_partition``, ``slot0_partition``, ``slot1_partition``, and
``scratch_partition`` nodes are defined for MCUboot, though not all MCUboot
configurations require all of them to be defined. See the `MCUboot
documentation`_ for more details.

The ``storage_partition`` node is defined for use by a file system or other
nonvolatile storage API.

.. _MCUboot documentation: https://mcuboot.com/

To get a numeric flash area ID from one of the child nodes of the
``partitions`` node:

#. take the node's ``label`` property value
#. lowercase it
#. convert all special characters to underscores (``_``)
#. pass the result **without quotes** to ``FLASH_AREA_ID()``

For example, the ``flash_area`` ID number for ``slot0_partition`` is
``FLASH_AREA_ID(image_0)``.

The same rules apply for other macros which take a "label", such as
:c:macro:`FLASH_AREA_OFFSET` and :c:macro:`FLASH_AREA_SIZE`. For example,
``FLASH_AREA_OFFSET(image_0)`` would return the start offset for
``slot0_partition`` within its flash device. This is determined by the node's
:ref:`devicetree-reg-property`, and in this case is 0x20000.

To get a pointer to the flash area structure and do something with it starting
with a devicetree label like ``"image-0"``, use something like this:

.. code-block:: c

   struct flash_area *my_area;
   int err = flash_area_open(FLASH_AREA_ID(image_0), &my_area);

   if (err != 0) {
   	handle_the_error(err);
   } else {
   	flash_area_read(my_area, ...);
   }

API Reference
*************

.. doxygengroup:: flash_area_api
