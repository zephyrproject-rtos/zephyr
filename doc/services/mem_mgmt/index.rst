.. _mem_mgmt_api:

Memory Attributes
#################

It is possible in the devicetree to mark the memory regions with attributes by
using the ``zephyr,memory-attr`` property. This property and the related memory
region can then be retrieved at run-time by leveraging a provided helper
library.

The set of general attributes that can be specified in the property are defined
and explained in :zephyr_file:`include/zephyr/dt-bindings/memory-attr/memory-attr.h`.

For example, to mark a memory region in the devicetree as non-volatile, cacheable,
out-of-order:

.. code-block:: devicetree

   mem: memory@10000000 {
       compatible = "mmio-sram";
       reg = <0x10000000 0x1000>;
       zephyr,memory-attr = <( DT_MEM_NON_VOLATILE | DT_MEM_CACHEABLE | DT_MEM_OOO )>;
   };

.. note::

   The ``zephyr,memory-attr`` usage does not result in any memory region
   actually created. When it is needed to create an actual section out of the
   devicetree defined memory region, it is possible to use the compatible
   :dtcompatible:`zephyr,memory-region` that will result (only when supported
   by the architecture) in a new linker section and region.

The ``zephyr,memory-attr`` property can also be used to set
architecture-specific and software-specific custom attributes that can be
interpreted at run time. This is leveraged, among other things, to create MPU
regions out of devicetree defined memory regions, for example:

.. code-block:: devicetree

   mem: memory@10000000 {
       compatible = "mmio-sram";
       reg = <0x10000000 0x1000>;
       zephyr,memory-region = "NOCACHE_REGION";
       zephyr,memory-attr = <( DT_MEM_ARM(ATTR_MPU_RAM_NOCACHE) )>;
   };

See :zephyr_file:`include/zephyr/dt-bindings/memory-attr/memory-attr-arm.h` and
:ref:`arm_cortex_m_developer_guide` for more details about MPU usage.

The conventional and recommended way to deal and manage with memory regions
marked with attributes is by using the provided ``mem-attr`` helper library by
enabling :kconfig:option:`CONFIG_MEM_ATTR`. When this option is enabled the
list of memory regions and their attributes are compiled in a user-accessible
array and a set of functions is made available that can be used to query, probe
and act on regions and attributes (see next section for more details).

.. note::

   The ``zephyr,memory-attr`` property is only a descriptive property of the
   capabilities of the associated memory region, but it does not result in any
   actual setting for the memory to be set. The user, code or subsystem willing
   to use this information to do some work (for example creating an MPU region
   out of the property) must use either the provided ``mem-attr`` library or
   the usual devicetree helpers to perform the required work / setting.

A test for the ``mem-attr`` library and its usage is provided in
``tests/subsys/mem_mgmt/mem_attr/``.

Migration guide from `zephyr,memory-region-mpu`
***********************************************

When the ``zephyr,memory-attr`` property was introduced, the
``zephyr,memory-region-mpu`` property was removed and deprecated.

The developers that are still using the deprecated property can move to the new
one by renaming the property and changing its value according to the following list:

.. code-block:: none

   "RAM"         -> <( DT_ARM_MPU(ATTR_MPU_RAM) )>
   "RAM_NOCACHE" -> <( DT_ARM_MPU(ATTR_MPU_RAM_NOCACHE) )>
   "FLASH"       -> <( DT_ARM_MPU(ATTR_MPU_FLASH) )>
   "PPB"         -> <( DT_ARM_MPU(ATTR_MPU_PPB) )>
   "IO"          -> <( DT_ARM_MPU(ATTR_MPU_IO) )>
   "EXTMEM"      -> <( DT_ARM_MPU(ATTR_MPU_EXTMEM) )>

API Reference
*************

.. doxygengroup:: memory_attr_interface
