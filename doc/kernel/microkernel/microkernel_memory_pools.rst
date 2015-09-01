.. _memory_pools:

Memory Pools
############

Concepts
********

The microkernel's memory pool objects provide dynamic allocation and
release of variable-size memory blocks.

Unlike a memory map, which only supports memory blocks of a single size,
a memory pool has the ability to support multiple memory block sizes.
It does this by subdividing blocks into smaller chunks whenever possible
to more closely match the actual needs of the requesting task.

Any number of memory pools can be defined in a microkernel system.
Each memory pool has a name that uniquely identifies it.
In addition, a memory pool defines minimum and maximum memory block sizes
(in bytes) and the number of maximum size blocks that the memory pool
contains.

A task that needs to use a memory block simply allocates it from a
memory pool. If a block of the desired size is unavailable, the task
may choose to wait for one to become available. Following a successful
allocation the :c:data:`pointer_to_data` field of the block descriptor
supplied by the task indicates the starting address of the memory block.
When the task is finished with a memory block, it must release the block
back to the memory pool that allocated it so that the block can be
reused.

Any number of tasks may wait on a memory pool simultaneously;
when a memory block becomes available it is given to the highest
priority task that has waited the longest.

When a request for memory is sufficiently smaller than an available
memory pool block, the memory pool will automatically split the block
block into 4 smaller blocks. The resulting smaller
blocks can also be split repeatedly, until a block just larger
than the needed size is available, or the minimum block size
(as specified in the MDEF file) is reached.
If the memory pool is unable to find an available block
that is at least the requested size, it will attempt to create
one by merging adjacent free blocks; if it is unable to create
a suitable block the request fails.

Although a memory pool uses efficient algorithms to manage its
blocks, splitting available blocks and merging free blocks
takes time and increases the overhead involved in allocating
a block. The larger the allowable number of splits, the larger
the overhead. The minimum and maximum block size parameters
specified for a pool can be used to control the amount of
splitting, and thus the amount of overhead.

Unlike a heap, more than one memory pool can be defined, if
needed. For example, different applications can utilize
different memory pools so that one application does not
allocate all of the available blocks.


Purpose
*******
Use memory pools to allocate memory in variable-size blocks.

Use memory pool blocks when sending data to a mailbox
asynchronously.


Usage
*****

Defining a Memory Pool
======================

The following parameters must be defined:

   *name*
          This specifies a unique name for the memory pool.

   *min_block_size*
          This specifies the minimimum memory block size in bytes.
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

Define the memory pool in the application's .MDEF file using the following
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

  task_mem_pool_alloc_wait(&block, MYPOOL, 80);

  memset(block.pointer_to_data, 0, 80);


Example: Requesting a Memory Block from a Pool with a Conditional Time-out
==========================================================================

This code waits up to 5 ticks for an 80 byte memory block to become
available and gives a warning if a suitable memory block is not obtained
in that time.

.. code-block:: c

  struct k_block block;

  if (task_mem_pool_alloc_wait_timeout(&block, MYPOOL, 80, 5) == RC_OK) {
      /* use memory block */
  } else {
      printf('Memory allocation timeout');
  }


Example: Requesting a Memory Block from a Pool with a No Blocking Condition
===========================================================================

This code gives an immediate warning when it can not satisfy the request for
a memory block of 80 bytes.

.. code-block:: c

  struct k_block block;

  if (task_mem_pool_alloc (&block, MYPOOL, 80) == RC_OK) {
      /* use memory block */
  } else {
      printf('Memory allocation timeout');
  }


Example: Freeing a Memory Block Back to a Pool
==============================================

This code releases a memory block back to a pool when it is no longer needed.

.. code-block:: c

  struct k_block block;

  task_mem_pool_alloc(&block, MYPOOL, size);
      /* use memory block */
  task_mem_pool_free(&block);


Example: Manually Defragmenting a Memory Pool
=============================================

This code instructs the memory pool to concatenate any unused memory blocks
that can be merged. Doing a full defragmentation of the entire memory pool
before allocating a number of memory blocks may be more efficient
than having to do an implicit partial defragmentation of the memory pool
each time a memory block allocation occurs.

.. code-block:: c

  task_mem_pool_defragment(MYPOOL);

APIs
****

The following Memory Pools APIs are provided by microkernel.h.

+----------------------------------------------+------------------------------+
| Call                                         | Description                  |
+==============================================+==============================+
| :c:func:`task_mem_pool_alloc()`              | Allocates a block from       |
|                                              | a memory pool.               |
+----------------------------------------------+------------------------------+
| :c:func:`task_mem_pool_alloc_wait()`         | Waits for a block of memory  |
|                                              | until it is available.       |
+----------------------------------------------+------------------------------+
| :c:func:`task_mem_pool_alloc_wait_timeout()` | Waits for a block of memory  |
|                                              | for the time period defined  |
|                                              | by the time-out parameter.   |
+----------------------------------------------+------------------------------+
| :c:func:`task_mem_pool_free()`               | Returns a block of memory    |
|                                              | to a memory pool.            |
+----------------------------------------------+------------------------------+
| :c:func:`task_mem_pool_defragment()`         | Defragments a memory pool.   |
+----------------------------------------------+------------------------------+
