.. _memory_management_shared_multi_heap:

Shared Multi Heap
#################

The shared multi-heap memory pool manager uses the multi-heap allocator to
manage a set of reserved memory regions with different capabilities /
attributes (cacheable, non-cacheable, etc...).

All the different regions can be added at run-time to the shared multi-heap
pool providing an opaque "attribute" value (an integer or enum value) that can
be used by drivers or applications to request memory with certain capabilities.

This framework is commonly used as follow:

1. At boot time some platform code initialize the shared multi-heap framework
   using :c:func:`shared_multi_heap_pool_init()` and add the memory regions to
   the pool with :c:func:`shared_multi_heap_add()`, possibly gathering the
   needed information for the regions from the DT.

2. Each memory region encoded in a :c:type:`shared_multi_heap_region`
   structure.  This structure is also carrying an opaque and user-defined
   integer value that is used to define the region capabilities (for example:
   cacheability, cpu affinity, etc...)

.. code-block:: c

   // Init the shared multi-heap pool
   shared_multi_heap_pool_init()

   // Fill the struct with the data for cacheable memory
   struct shared_multi_heap_region cacheable_r0 = {
        .addr = addr_r0,
        .size = size_r0,
        .attr = SMH_REG_ATTR_CACHEABLE,
   };

   // Add the region to the pool
   shared_multi_heap_add(&cacheable_r0, NULL);

   // Add another cacheable region
   struct shared_multi_heap_region cacheable_r1 = {
        .addr = addr_r1,
        .size = size_r1,
        .attr = SMH_REG_ATTR_CACHEABLE,
   };

   shared_multi_heap_add(&cacheable_r0, NULL);

   // Add a non-cacheable region
   struct shared_multi_heap_region non_cacheable_r2 = {
        .addr = addr_r2,
        .size = size_r2,
        .attr = SMH_REG_ATTR_NON_CACHEABLE,
   };

   shared_multi_heap_add(&non_cacheable_r2, NULL);

3. When a driver or application needs some dynamic memory with a certain
   capability, it can use :c:func:`shared_multi_heap_alloc()` (or the aligned
   version) to request the memory by using the opaque parameter to select the
   correct set of attributes for the needed memory. The framework will take
   care of selecting the correct heap (thus memory region) to carve memory
   from, based on the opaque parameter and the runtime state of the heaps
   (available memory, heap state, etc...)

.. code-block:: c

   // Allocate 4K from cacheable memory
   shared_multi_heap_alloc(SMH_REG_ATTR_CACHEABLE, 0x1000);

   // Allocate 4K from non-cacheable memory
   shared_multi_heap_alloc(SMH_REG_ATTR_NON_CACHEABLE, 0x1000);

Adding new attributes
*********************

The API does not enforce any attributes, but at least it defines the two most
common ones: :c:enum:`SMH_REG_ATTR_CACHEABLE` and :c:enum:`SMH_REG_ATTR_NON_CACHEABLE`

.. doxygengroup:: shared_multi_heap
   :project: Zephyr
