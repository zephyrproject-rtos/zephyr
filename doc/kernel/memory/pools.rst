.. _memory_pools_v2:

Memory Pools
############

A :dfn:`memory pool` is a kernel object that allows memory blocks
to be dynamically allocated from a designated memory region.
The memory blocks in a memory pool can be of any size,
thereby reducing the amount of wasted memory when an application
needs to allocate storage for data structures of different sizes.
The memory pool uses a "buddy memory allocation" algorithm
to efficiently partition larger blocks into smaller ones,
allowing blocks of different sizes to be allocated and released efficiently
while limiting memory fragmentation concerns.

.. contents::
    :local:
    :depth: 2

Concepts
********

Any number of memory pools can be defined. Each memory pool is referenced
by its memory address.

A memory pool has the following key properties:

* A **minimum block size**, measured in bytes.
  It must be at least 4X bytes long, where X is greater than 0.

* A **maximum block size**, measured in bytes.
  This should be a power of 4 times larger than the minimum block size.
  That is, "maximum block size" must equal "minimum block size" times 4^Y,
  where Y is greater than or equal to zero.

* The **number of maximum-size blocks** initially available.
  This must be greater than zero.

* A **buffer** that provides the memory for the memory pool's blocks.
  This must be at least "maximum block size" times
  "number of maximum-size blocks" bytes long.

The memory pool's buffer must be aligned to an N-byte boundary, where
N is a power of 2 larger than 2 (i.e. 4, 8, 16, ...). To ensure that
all memory blocks in the buffer are similarly aligned to this boundary,
the minimum block size must also be a multiple of N.

A thread that needs to use a memory block simply allocates it from a memory
pool. Following a successful allocation, the :c:data:`data` field
of the block descriptor supplied by the thread indicates the starting address
of the memory block. When the thread is finished with a memory block,
it must release the block back to the memory pool so the block can be reused.

If a block of the desired size is unavailable, a thread can optionally wait
for one to become available.
Any number of threads may wait on a memory pool simultaneously;
when a suitable memory block becomes available, it is given to
the highest-priority thread that has waited the longest.

Unlike a heap, more than one memory pool can be defined, if needed. For
example, different applications can utilize different memory pools; this
can help prevent one application from hijacking resources to allocate all
of the available blocks.

Internal Operation
==================

A memory pool's buffer is an array of maximum-size blocks,
with no wasted space between the blocks.
Each of these "level 0" blocks is a *quad-block* that can be
partitioned into four smaller "level 1" blocks of equal size, if needed.
Likewise, each level 1 block is itself a quad-block that can be partitioned
into four smaller "level 2" blocks in a similar way, and so on.
Thus, memory pool blocks can be recursively partitioned into quarters
until blocks of the minimum size are obtained,
at which point no further partitioning can occur.

A memory pool keeps track of how its buffer space has been partitioned
using an array of *block set* data structures. There is one block set
for each partitioning level supported by the pool, or (to put it another way)
for each block size. A block set keeps track of all free blocks of its
associated size using an array of *quad-block status* data structures.

When an application issues a request for a memory block,
the memory pool first determines the size of the smallest block
that will satisfy the request, and examines the corresponding block set.
If the block set contains a free block, the block is marked as used
and the allocation process is complete.
If the block set does not contain a free block,
the memory pool attempts to create one automatically by splitting a free block
of a larger size or by merging free blocks of smaller sizes;
if a suitable block can't be created, the allocation request fails.

The memory pool's merging algorithm cannot combine adjacent free
blocks of different sizes, nor can it merge adjacent free blocks of
the same size if they belong to different parent quad-blocks. As a
consequence, memory fragmentation issues can still be encountered when
using a memory pool.

When an application releases a previously allocated memory block it is
combined synchronously with its three "partner" blocks if possible,
and recursively so up through the levels.  This is done in constant
time, and quickly, so no manual "defragmentation" management is
needed.

Implementation
**************

Defining a Memory Pool
======================

A memory pool is defined using a variable of type :c:type:`struct k_mem_pool`.
However, since a memory pool also requires a number of variable-size data
structures to represent its block sets and the status of its quad-blocks,
the kernel does not support the runtime definition of a memory pool.
A memory pool can only be defined and initialized at compile time
by calling :c:macro:`K_MEM_POOL_DEFINE`.

The following code defines and initializes a memory pool that has 3 blocks
of 4096 bytes each, which can be partitioned into blocks as small as 64 bytes
and is aligned to a 4-byte boundary.
(That is, the memory pool supports block sizes of 4096, 1024, 256,
and 64 bytes.)
Observe that the macro defines all of the memory pool data structures,
as well as its buffer.

.. code-block:: c

    K_MEM_POOL_DEFINE(my_pool, 64, 4096, 3, 4);

Allocating a Memory Block
=========================

A memory block is allocated by calling :cpp:func:`k_mem_pool_alloc()`.

The following code builds on the example above, and waits up to 100 milliseconds
for a 200 byte memory block to become available, then fills it with zeroes.
A warning is issued if a suitable block is not obtained.

Note that the application will actually receive a 256 byte memory block,
since that is the closest matching size supported by the memory pool.

.. code-block:: c

    struct k_mem_block block;

    if (k_mem_pool_alloc(&my_pool, &block, 200, 100) == 0)) {
        memset(block.data, 0, 200);
	...
    } else {
        printf("Memory allocation time-out");
    }

Memory blocks may also be allocated with :cpp:func:`malloc()`-like semantics
using :cpp:func:`k_mem_pool_malloc()`. Such allocations must be freed with
:cpp:func:`k_free()`.

Releasing a Memory Block
========================

A memory block is released by calling either :cpp:func:`k_mem_pool_free()`
or :cpp:func:`k_free()`, depending on how it was allocated.

The following code builds on the example above, and allocates a 75 byte
memory block, then releases it once it is no longer needed. (A 256 byte
memory block is actually used to satisfy the request.)

.. code-block:: c

    struct k_mem_block block;

    k_mem_pool_alloc(&my_pool, &block, 75, K_FOREVER);
    ... /* use memory block */
    k_mem_pool_free(&block);

Thread Resource Pools
*********************

Certain kernel APIs may need to make heap allocations on behalf of the
calling thread. For example, some initialization APIs for objects like
pipes and message queues may need to allocate a private kernel-side buffer,
or objects like queues may temporarily allocate kernel data structures
as items are placed in the queue.

Such memory allocations are drawn from memory pools that are assigned to
a thread. By default, a thread in the system has no resource pool and
any allocations made on its behalf will fail. The supervisor-mode only
:cpp:func:`k_thread_resource_pool_assign()` will associate any implicit
kernel-side allocations to the target thread with the provided memory pool,
and any children of that thread will inherit this assignment.

If a system heap exists, threads may alternatively have their resources
drawn from it using the :cpp:func:`k_thread_system_pool_assign()` API.

Suggested Uses
**************

Use a memory pool to allocate memory in variable-size blocks.

Use memory pool blocks when sending large amounts of data from one thread
to another, to avoid unnecessary copying of the data.

API Reference
*************

.. doxygengroup:: mem_pool_apis
   :project: Zephyr

