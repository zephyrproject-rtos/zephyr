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

Migration guide from ``zephyr,memory-region-mpu``
*************************************************

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

Memory Attributes Heap Allocator
********************************

It is possible to leverage the memory attribute property ``zephyr,memory-attr``
to define and create a set of memory heaps from which the user can allocate
memory from with certain attributes / capabilities.

When the :kconfig:option:`CONFIG_MEM_ATTR_HEAP` is set, every region marked
with one of the memory attributes listed in
:zephyr_file:`include/zephyr/dt-bindings/memory-attr/memory-attr-sw.h` is added
to a pool of memory heaps used for dynamic allocation of memory buffers with
certain attributes.

Here a non exhaustive list of possible attributes:

.. code-block:: none

   DT_MEM_SW_ALLOC_CACHE
   DT_MEM_SW_ALLOC_NON_CACHE
   DT_MEM_SW_ALLOC_DMA

For example we can define several memory regions with different attributes and
use the appropriate attribute to indicate that it is possible to dynamically
allocate memory from those regions:

.. code-block:: devicetree

   mem_cacheable: memory@10000000 {
       compatible = "mmio-sram";
       reg = <0x10000000 0x1000>;
       zephyr,memory-attr = <( DT_MEM_CACHEABLE | DT_MEM_SW_ALLOC_CACHE )>;
   };

   mem_non_cacheable: memory@20000000 {
       compatible = "mmio-sram";
       reg = <0x20000000 0x1000>;
       zephyr,memory-attr = <( DT_MEM_NON_CACHEABLE | ATTR_SW_ALLOC_NON_CACHE )>;
   };

   mem_cacheable_big: memory@30000000 {
       compatible = "mmio-sram";
       reg = <0x30000000 0x10000>;
       zephyr,memory-attr = <( DT_MEM_CACHEABLE | DT_MEM_OOO | DT_MEM_SW_ALLOC_CACHE )>;
   };

   mem_cacheable_dma: memory@40000000 {
       compatible = "mmio-sram";
       reg = <0x40000000 0x10000>;
       zephyr,memory-attr = <( DT_MEM_CACHEABLE      | DT_MEM_DMA |
                               DT_MEM_SW_ALLOC_CACHE | DT_MEM_SW_ALLOC_DMA )>;
   };

The user can then dynamically carve memory out of those regions using the
provided functions, the library will take care of allocating memory from the
correct heap depending on the provided attribute and size:

.. code-block:: c

   // Init the pool
   mem_attr_heap_pool_init();

   // Allocate 0x100 bytes of cacheable memory from `mem_cacheable`
   block = mem_attr_heap_alloc(DT_MEM_SW_ALLOC_CACHE, 0x100);

   // Allocate 0x200 bytes of non-cacheable memory aligned to 32 bytes
   // from `mem_non_cacheable`
   block = mem_attr_heap_aligned_alloc(ATTR_SW_ALLOC_NON_CACHE, 0x100, 32);

   // Allocate 0x100 bytes of cacheable and dma-able memory from `mem_cacheable_dma`
   block = mem_attr_heap_alloc(DT_MEM_SW_ALLOC_CACHE | DT_MEM_SW_ALLOC_DMA, 0x100);

When several regions are marked with the same attributes, the memory is allocated:

1. From the regions where the ``zephyr,memory-attr`` property has the requested
   property (or properties).

2. Among the regions as at point 1, from the smallest region if there is any
   unallocated space left for the requested size

3. If there is not enough space, from the next bigger region able to
   accommodate the requested size

The following example shows the point 3:

.. code-block:: c

   // This memory is allocated from `mem_non_cacheable`
   block = mem_attr_heap_alloc(DT_MEM_SW_ALLOC_NON_CACHE, 0x100);

   // This memory is allocated from `mem_cacheable_big`
   block = mem_attr_heap_alloc(DT_MEM_SW_ALLOC_CACHE, 0x5000);

.. note::

    The framework is assuming that the memory regions used to create the heaps
    are usable by the code and available at init time. The user must take of
    initializing and setting the memory area before calling
    :c:func:`mem_attr_heap_pool_init`.

    That means that the region must be correctly configured in terms of MPU /
    MMU (if needed) and that an actual heap can be created out of it, for
    example by leveraging the ``zephyr,memory-region`` property to create a
    proper linker section to accommodate the heap.

API Reference
*************

.. doxygengroup:: memory_attr_interface
.. doxygengroup:: memory_attr_heap
