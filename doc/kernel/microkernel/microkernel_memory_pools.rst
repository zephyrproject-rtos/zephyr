.. _microkernel_memory_pools:

Memory Pools
############

Concepts
********

The microkernel's :dfn:`memory pool` objects provide dynamic allocation and
release of variable-size memory blocks.

Unlike :ref:`memory map <microkernel_memory_map>` objects, which support
memory blocks of only a *single* size, a memory pool can support memory blocks
of *various* sizes. The memory pool does this by subdividing blocks into smaller
chunks, where possible, to more closely match the actual needs of a requesting
task.

Any number of memory pools can be defined in a microkernel system. Each memory
pool has:

* A **name** that uniquely identifies it.
* A **minimum** and **maximum** block size, in bytes, of memory blocks
  within the pool.
* The **number of maximum-size memory blocks** initially available.

A task that needs to use a memory block simply allocates it from a memory
pool. When a block of the desired size is unavailable, the task can wait
for one to become available. Following a successful allocation, the
:c:data:`pointer_to_data` field of the block descriptor supplied by the
task indicates the starting address of the memory block. When the task is
finished with a memory block, it must release the block back to the memory
pool that allocated it so that the block can be reused.

Any number of tasks can wait on a memory pool simultaneously; when a
memory block becomes available, it is given to the highest-priority task
that has waited the longest.

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

Purpose
*******
Use memory pools to allocate memory in variable-size blocks.

Use memory pool blocks when sending data to a mailbox asynchronously.

Usage
*****

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

Example: Requesting a Memory Block from a Pool with No Conditions
=================================================================

This code waits indefinitely for an 80 byte memory block to become
available, then fills it with zeroes.

.. code-block:: c

  struct k_block block;

  task_mem_pool_alloc(&block, MYPOOL, 80, TICKS_UNLIMITED);

  memset(block.pointer_to_data, 0, 80);

Example: Requesting a Memory Block from a Pool with a Conditional Time-out
==========================================================================

This code waits up to 5 ticks for an 80 byte memory block to become
available and gives a warning if a suitable memory block is not obtained
in that time.

.. code-block:: none

   struct k_block block;

   if (task_mem_pool_alloc(:&block, MYPOOL, 80, 5) == RC_OK) {
           /* use memory block */
   } else {
           printf('Memory allocation timeout');
   }

Example: Requesting a Memory Block from a Pool with a No-Blocking Condition
===========================================================================

This code gives an immediate warning when it can not satisfy the request for
a memory block of 80 bytes.

.. code-block:: none

   struct k_block block;

   if (task_mem_pool_alloc (&block, MYPOOL, 80, TICKS_NONE) == RC_OK) {
           /* use memory block */
   } else {
           printf('Memory allocation timeout');
   }

Example: Freeing a Memory Block Back to a Pool
==============================================

This code releases a memory block back to a pool when it is no longer needed.

.. code-block:: c

  struct k_block block;

  task_mem_pool_alloc(&block, MYPOOL, size, TICKS_NONE);
      /* use memory block */
  task_mem_pool_free(&block);

Example: Manually Defragmenting a Memory Pool
=============================================

This code instructs the memory pool to concatenate any unused memory blocks
that can be merged. Doing a full defragmentation of the entire memory pool
before allocating a number of memory blocks may be more efficient than doing
an implicit partial defragmentation of the memory pool each time a memory
block allocation occurs.

.. code-block:: c

  task_mem_pool_defragment(MYPOOL);

APIs
****

Memory Pools APIs provided by :file:`microkernel.h`
===================================================

:cpp:func:`task_mem_pool_alloc()`
   Wait for a block of memory; wait the period of time defined by the time-out
   parameter.

:cpp:func:`task_mem_pool_free()`
   Return a block of memory to a memory pool.

:cpp:func:`task_mem_pool_defragment()`
   Defragment a memory pool.
