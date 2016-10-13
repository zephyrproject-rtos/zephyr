.. _memory_maps_v2:

Memory Maps
###########

A :dfn:`memory map` is a kernel object that allows memory blocks
to be dynamically allocated from a designated memory region.
All memory blocks in a memory map have a single fixed size,
allowing them to be allocated and released efficiently
and avoiding memory fragmentation concerns.

.. contents::
    :local:
    :depth: 2

Concepts
********

Any number of memory maps can be defined. Each memory map is referenced
by its memory address.

A memory map has the following key properties:

* The **block size** of each block, measured in bytes.
  It must be at least 4 bytes long.

* The **number of blocks** available for allocation.
  It must be greater than zero.

* A **buffer** that provides the memory for the memory map's blocks.
  It must be at least "block size" times "number of blocks" bytes long.

A thread that needs to use a memory block simply allocates it from a memory
map. When the thread finishes with a memory block,
it must release the block back to the memory map so the block can be reused.

If all the blocks are currently in use, a thread can optionally wait
for one to become available.
Any number of thread may wait on an empty memory map simultaneously;
when a memory block becomes available, it is given to the highest-priority
thread that has waited the longest.

Unlike a heap, more than one memory map can be defined, if needed. This
allows for a memory map with smaller blocks and others with larger-sized
blocks. Alternatively, a memory pool object may be used.

Internal Operation
==================

A memory map's buffer is an array of fixed-size blocks,
with no wasted space between the blocks.

The memory map keeps track of unallocated blocks using a linked list;
the first 4 bytes of each unused block provide the necessary linkage.

Implementation
**************

Defining a Memory Map
=====================

A memory map is defined using a variable of type :c:type:`struct k_mem_map`.
It must then be initialized by calling :cpp:func:`k_mem_map_init()`.

The following code defines and initializes a memory map that has 6 blocks
of 400 bytes each.

.. code-block:: c

    struct k_mem_map my_map;
    char my_map_buffer[6 * 400];

    k_mem_map_init(&my_map, 6, 400, my_map_buffer);

Alternatively, a memory map can be defined and initialized at compile time
by calling :c:macro:`K_MEM_MAP_DEFINE()`.

The following code has the same effect as the code segment above. Observe
that the macro defines both the memory map and its buffer.

.. code-block:: c

    K_MEM_MAP_DEFINE(my_map, 6, 400);

Allocating a Memory Block
=========================

A memory block is allocated by calling :cpp:func:`k_mem_map_alloc()`.

The following code builds on the example above, and waits up to 100 milliseconds
for a memory block to become available, then fills it with zeroes.
A warning is printed if a suitable block is not obtained.

.. code-block:: c

    char *block_ptr;

    if (k_mem_map_alloc(&my_map, &block_ptr, 100) == 0)) {
        memset(block_ptr, 0, 400);
	...
    } else {
        printf("Memory allocation time-out");
    }

Releasing a Memory Block
========================

A memory block is released by calling :cpp:func:`k_mem_map_free()`.

The following code builds on the example above, and allocates a memory block,
then releases it once it is no longer needed.

.. code-block:: c

    char *block_ptr;

    k_mem_map_alloc(&my_map, &block_ptr, K_FOREVER);
    ... /* use memory block pointed at by block_ptr */
    k_mem_map_free(&my_map, &block_ptr);

Suggested Uses
**************

Use a memory map to allocate and free memory in fixed-size blocks.

Use memory map blocks when sending large amounts of data from one thread
to another, to avoid unnecessary copying of the data.

Configuration Options
*********************

Related configuration options:

* None.

APIs
****

The following memory map APIs are provided by :file:`kernel.h`:

* :cpp:func:`k_mem_map_init()`
* :cpp:func:`k_mem_map_alloc()`
* :cpp:func:`k_mem_map_free()`
* :cpp:func:`k_mem_map_num_used_get()`
* :cpp:func:`k_mem_map_num_free_get()`
