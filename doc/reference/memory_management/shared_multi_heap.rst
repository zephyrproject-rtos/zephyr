.. _memory_management_shared_multi_heap:

Shared Multi Heap
#################

The shared multi-heap memory pool manager uses the multi-heap allocator to
manage a set of reserved memory regions with different capabilities /
attributes (cacheable, non-cacheable, etc...) defined in the DT.

The user can request allocation from the shared pool specifying the capability
/ attribute of interest for the memory (cacheable / non-cacheable memory,
etc...).

The different heaps with their attributes available in the shared pool are
defined into the DT file leveraging the ``reserved-memory`` nodes.

This is a DT example declaring three different memory regions with different
cacheability attributes: ``cacheable`` and ``non-cacheable``

.. code-block:: devicetree

   / {
        reserved-memory {
                compatible = "reserved-memory";
                #address-cells = <1>;
                #size-cells = <1>;

                res0: reserved@42000000 {
                        compatible = "shared-multi-heap";
                        reg = <0x42000000 0x1000>;
                        capability = "cacheable";
                        label = "res0";
                };

                res1: reserved@43000000 {
                        compatible = "shared-multi-heap";
                        reg = <0x43000000 0x2000>;
                        capability = "non-cacheable";
                        label = "res1";
                };

                res2: reserved2@44000000 {
                        compatible = "shared-multi-heap";
                        reg = <0x44000000 0x3000>;
                        capability = "cacheable";
                        label = "res2";
                };
        };

The user can then request 4K from heap memory ``cacheable`` or
``non-cacheable`` using the provided APIs:

.. code-block:: c

   // Allocate 4K from cacheable memory
   shared_multi_heap_alloc(SMH_REG_ATTR_CACHEABLE, 0x1000);

   // Allocate 4K from non-cacheable
   shared_multi_heap_alloc(SMH_REG_ATTR_NON_CACHEABLE, 0x1000);

The backend implementation will allocate the memory region from the heap with
the correct attribute and using the region able to accommodate the required size.

Special handling for MMU/MPU
****************************

For MMU/MPU enabled platform sometimes it is required to setup and configure
the memory regions before these are added to the managed pool. This is done at
init time using the :c:func:`shared_multi_heap_pool_init()` function that is
accepting a :c:type:`smh_init_reg_fn_t` callback function. This callback will
be called for each memory region at init time and it can be used to correctly
map the region before this is considered valid and accessible.

Adding new attributes
*********************

Currently only two memory attributes are supported: ``cacheable`` and
``non-cacheable``. To add a new attribute:

1. Add the new ``enum`` for the attribute in the :c:enum:`smh_reg_attr`
2. Add the corresponding attribute name in :file:`shared-multi-heap.yaml`

.. doxygengroup:: shared_multi_heap
   :project: Zephyr
