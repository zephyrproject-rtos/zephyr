.. _sys_mem_blocks:

Memory Blocks Allocator
#######################

The Memory Blocks Allocator allows memory blocks to be dynamically
allocated from a designated memory region, where:

* All memory blocks have a single fixed size.

* Multiple blocks can be allocated or freed at the same time.

* A group of blocks allocated together may not be contiguous.
  This is useful for operations such as scatter-gather DMA transfers.

* Bookkeeping of allocated blocks is done outside of the associated
  buffer (unlike memory slab). This allows the buffer to reside in
  memory regions where these can be powered down to conserve energy.

.. contents::
    :local:
    :depth: 2

Concepts
********

Any number of Memory Blocks Allocator can be defined (limited only by
available RAM). Each allocator is referenced by its memory address.

A memory blocks allocator has the following key properties:

* The **block size** of each block, measured in bytes.
  It must be at least 4N bytes long, where N is greater than 0.

* The **number of blocks** available for allocation.
  It must be greater than zero.

* A **buffer** that provides the memory for the memory slab's blocks.
  It must be at least "block size" times "number of blocks" bytes long.

* A **blocks bitmap** to keep track of which block has been allocated.

The buffer must be aligned to an N-byte boundary, where N is a power of 2
larger than 2 (i.e. 4, 8, 16, ...). To ensure that all memory blocks in
the buffer are similarly aligned to this boundary, the block size must
also be a multiple of N.

Due to the use of internal bookkeeping structures and their creation,
each memory blocks allocator must be declared and defined at compile time.

Internal Operation
==================

Each buffer associated with an allocator is an array of fixed-size blocks,
with no wasted space between the blocks.

The memory blocks allocator keeps track of unallocated blocks using
a bitmap.

Memory Blocks Allocator
***********************

Internally, the memory blocks allocator uses a bitmap to keep track of
which blocks have been allocated. Each allocator, utilizing
the ``sys_bitarray`` interface, gets memory blocks one by one from
the backing buffer up to the requested number of blocks.
All the metadata about an allocator is stored outside of the backing
buffer. This allows the memory region of the backing buffer to be
powered down to conserve energy, as the allocator code never touches
the content of the buffer.

Multi Memory Blocks Allocator Group
***********************************

The Multi Memory Blocks Allocator Group utility functions provide
a convenient to manage a group of allocators. A custom allocator
choosing function is used to choose which allocator to use among
this group.

An allocator group should be initialized at runtime via
:c:func:`sys_multi_mem_blocks_init`. Each allocator can then be
added via :c:func:`sys_multi_mem_blocks_add_allocator`.

To allocate memory blocks from group,
:c:func:`sys_multi_mem_blocks_alloc` is called with an opaque
"configuration" parameter. This parameter is passed directly to
the allocator choosing function so that an appropriate allocator
can be chosen. After an allocator is chosen, memory blocks are
allocated via :c:func:`sys_mem_blocks_alloc`.

Allocated memory blocks can be freed via
:c:func:`sys_multi_mem_blocks_free`. The caller does not need to
pass a configuration parameter. The allocator code matches
the passed in memory addresses to find the correct allocator
and then memory blocks are freed via :c:func:`sys_mem_blocks_free`.

Usage
*****

Defining a Memory Blocks Allocator
==================================

A memory blocks allocator is defined using a variable of type
:c:type:`sys_mem_blocks_t`. It needs to be defined and initialized
at compile time by calling :c:macro:`SYS_MEM_BLOCKS_DEFINE`.

The following code defines and initializes a memory blocks allocator
which has 4 blocks that are 64 bytes long, each of which is aligned
to a 4-byte boundary:

.. code-block:: c

   SYS_MEM_BLOCKS_DEFINE(allocator, 64, 4, 4);

Similarly, you can define a memory slab in private scope:

.. code-block:: c

   SYS_MEM_BLOCKS_DEFINE_STATIC(static_allocator, 64, 4, 4);

A pre-defined buffer can also be provided to the allocator where
the buffer can be placed separately. Note that the alignment of
the buffer needs to be done at its definition.

.. code-block:: c

   uint8_t __aligned(4) backing_buffer[64 * 4];
   SYS_MEM_BLOCKS_DEFINE_WITH_EXT_BUF(allocator, 64, 4, backing_buffer);

Allocating Memory Blocks
========================

Memory blocks can be allocated by calling :c:func:`sys_mem_blocks_alloc`.

.. code-block:: c

   int ret;
   uintptr_t blocks[2];

   ret = sys_mem_blocks_alloc(allocator, 2, blocks);

If ``ret == 0``, the array ``blocks`` will contain an array of memory
addresses pointing to the allocated blocks.

Releasing a Memory Block
========================

Memory blocks are released by calling :c:func:`sys_mem_blocks_free`.

The following code builds on the example above which allocates 2 memory blocks,
then releases them once they are no longer needed.

.. code-block:: c

   int ret;
   uintptr_t blocks[2];

   ret = sys_mem_blocks_alloc(allocator, 2, blocks);
   ... /* perform some operations on the allocated memory blocks */
   ret = sys_mem_blocks_free(allocator, 2, blocks);

Using Multi Memory Blocks Allocator Group
=========================================

The following code demonstrates how to initialize an allocator group:

.. code-block:: c

   sys_mem_blocks_t *choice_fn(struct sys_multi_mem_blocks *group, void *cfg)
   {
       ...
   }

   SYS_MEM_BLOCKS_DEFINE(allocator0, 64, 4, 4);
   SYS_MEM_BLOCKS_DEFINE(allocator1, 64, 4, 4);

   static sys_multi_mem_blocks_t alloc_group;

   sys_multi_mem_blocks_init(&alloc_group, choice_fn);
   sys_multi_mem_blocks_add_allocator(&alloc_group, &allocator0);
   sys_multi_mem_blocks_add_allocator(&alloc_group, &allocator1);

To allocate and free memory blocks from the group:

.. code-block:: c

   int ret;
   uintptr_t blocks[1];
   size_t blk_size;

   ret = sys_multi_mem_blocks_alloc(&alloc_group, UINT_TO_POINTER(0),
                                    1, blocks, &blk_size);

   ret = sys_multi_mem_blocks_free(&alloc_group, 1, blocks);

API Reference
*************

.. doxygengroup:: mem_blocks_apis
