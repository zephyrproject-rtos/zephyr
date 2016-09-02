.. _memory_pools_v2:

Memory Pools [TBD]
##################

A :dfn:`memory pool` is a kernel object that allows variable-size memory blocks
to be dynamically allocated from a designated memory region.

.. contents::
    :local:
    :depth: 2

Concepts
********

Unlike :ref:`memory map <microkernel_memory_maps>` objects, which support
memory blocks of only a *single* size, a memory pool can support memory blocks
of *various* sizes. The memory pool does this by subdividing blocks into smaller
chunks, where possible, to more closely match the actual needs of a requesting
task.

Any number of memory pools can be defined. Each memory pool is referenced
by its memory address.

A memory pool has the following key properties:

* A **minimum** and **maximum** block size, measured in bytes.
* The **number of maximum-size memory blocks** initially available.

The number of blocks and block size values must be greater than zero.
The block size must be defined as a multiple of the word size.

A thread that needs to use a memory block simply allocates it from a memory
pool. Following a successful allocation, the :c:data:`pointer_to_data` field
of the block descriptor supplied by the thread indicates the starting address
of the memory block. When the thread is finished with a memory block,
it must release the block back to the memory pool so the block can be reused.

If a block of the desired size is unavailable, a thread can optionally wait
for one to become available.
Any number of threads may wait on a memory pool simultaneously;
when a suitable memory block becomes available, it is given to
the highest-priority task that has waited the longest.

When a request for memory is sufficiently smaller than an available
memory pool block, the memory pool will automatically split the block into
4 smaller blocks. The resulting smaller blocks can also be split repeatedly,
until a block just larger than the needed size is available, or the minimum
block size, as specified in the MDEF, is reached.

If the memory pool cannot find an available block that is at least
the requested size, it will attempt to create one by merging adjacent
free blocks. If a suitable block can't be created, the request fails.

Although a memory pool uses efficient algorithms to manage its blocks,
the splitting of available blocks and merging of free blocks takes time
and increases overhead block allocation. The larger the allowable
number of splits, the larger the overhead. However, the minimum and maximum
block-size parameters specified for a pool can be used to control the amount
of splitting, and thus the amount of overhead.

Unlike a heap, more than one memory pool can be defined, if needed. For
example, different applications can utilize different memory pools; this
can help prevent one application from hijacking resources to allocate all
of the available blocks.

Implementation
**************

Defining a Memory Pool
======================

The following parameters must be defined:

   *name*
          This specifies a unique name for the memory pool.

   *min_block_size*
          This specifies the minimum memory block size in bytes.
          It should be a multiple of the processor's word size.

   *max_block_size*
          This specifies the maximum memory block size in bytes.
          It should be a power of 4 times larger than *minBlockSize*;
          therefore, maxBlockSize = minBlockSize * 4^n, where n>=0.

   *num_max*
          This specifies the number of maximum size memory blocks
          available at startup.

Public Memory Pool
------------------

Define the memory pool in the application's MDEF with the following
syntax:

.. code-block:: console

   POOL name min_block_size max_block_size num_max

For example, the file :file:`projName.mdef` defines two memory pools
as follows:

.. code-block:: console

    % POOL NAME            MIN  MAX     NMAX
    % =======================================
      POOL MY_POOL         32   8192      1
      POOL SECOND_POOL_ID  64   1024      5

A public memory pool can be referenced by name from any source file that
includes the file :file:`zephyr.h`.

.. note::
   Private memory pools are not supported by the Zephyr kernel.

Allocating a Memory Block
=========================

A memory block is allocated by calling :cpp:func:`k_mem_pool_alloc()`.

The following code waits up to 100 milliseconds for a 256 byte memory block
to become available, then fills it with zeroes. A warning is issued
if a suitable block is not obtained.

.. code-block:: c

    struct k_mem_block block;

    if (k_mem_pool_alloc(&my_pool, 256, &block, 100) == 0)) {
        memset(block.pointer_to_data, 0, 256);
	...
    } else {
        printf("Memory allocation time-out");
    }

Freeing a Memory Block
======================

A memory block is released by calling :cpp:func:`k_mem_pool_free()`.

The following code allocates a memory block, then releases it once
it is no longer needed.

.. code-block:: c

    struct k_mem_block block;

    k_mem_pool_alloc(&my_pool, size, &block, K_FOREVER);
     /* use memory block */
    k_mem_pool_free(&block);

Manually Defragmenting a Memory Pool
====================================

This code instructs the memory pool to concatenate any unused memory blocks
that can be merged. Doing a full defragmentation of the entire memory pool
before allocating a number of memory blocks may be more efficient than doing
an implicit partial defragmentation of the memory pool each time a memory
block allocation occurs.

.. code-block:: c

    k_mem_pool_defragment(&my_pool);

Suggested Uses
**************

Use a memory pool to allocate memory in variable-size blocks.

Use memory pool blocks when sending large amounts of data from one thread
to another, to avoid unnecessary copying of the data.

APIs
****

The following memory pool APIs are provided by :file:`kernel.h`:

* :cpp:func:`k_mem_pool_alloc()`
* :cpp:func:`k_mem_pool_free()`
* :cpp:func:`k_mem_pool_defragment()`
