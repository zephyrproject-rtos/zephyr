.. _memory_maps:

Memory Maps
###########

Concepts
********

The microkernel's memory map objects provide dynamic allocation and
release of fixed-size memory blocks.

Any number of memory maps can be defined in a microkernel system.
Each memory map has a name that uniquely identifies it.
In addition, a memory map defines the number of blocks it contains
and the size of each block in bytes.
The number of blocks and block size values cannot be zero.
The block size must be a multiple of the word size on most processors.

A task that needs to use a memory block simply allocates it from
a memory map. When all the blocks are currently in use, the task may
choose to wait for one to become available. When the task is finished
with a memory block, it must release the block back to the memory map
that allocated it so that the block can be reused.

Any number of tasks may wait on an empty memory map simultaneously;
when a memory block becomes available it is given to the
highest priority task that has waited the longest.

The microkernel manages memory blocks in an efficient and deterministic
manner that eliminates the risk of memory fragmentation problems
which can arise when using variable-size blocks.

Unlike a heap, more than one memory map can be defined, if needed. This
allows for a memory map with smaller blocks and others with larger-sized
blocks. (Alternatively, a memory pool object may be used.)


Purpose
*******

Use a memory map to allocate and free memory in fixed-size blocks.


Usage
*****

Defining a Memory Map
=====================

The following parameters must be defined:

   *name*
          This specifies a unique name for the memory map.

   *num_blocks*
          This specifies the number of memory blocks in the memory map.

   *block_size*
          This specifies the size (in bytes) of each memory block.


Public Memory Map
-----------------

Define the memory map in the application's .MDEF file using the following
syntax:

.. code-block:: console

   MAP name num_blocks block_size

For example, the file :file:`projName.mdef` defines a pair of memory maps
as follows:

.. code-block:: console

   % MAP  NAME      NUMBLOCKS     BLOCKSIZE
   % ======================================
     MAP  MYMAP     4             1024
     MAP  YOURMAP   6             200

A public memory map can be referenced from any source file that includes
the file :file:`zephyr.h`.


Private Memory Map
------------------

Define the memory map in a source file using the following syntax:

.. code-block:: c

   DEFINE_MEMORY_MAP(name, num_blocks, block_size);

For example, the following code defines a private memory map named
``PRIV_MEM_MAP``.

.. code-block:: c

   DEFINE_MEMORY_MAP(PRIV_MEM_MAP, 6, 200);

To utilize this memory map from a different source file use
the following syntax:

.. code-block:: c

   extern const kmemory_map_t PRIV_MEM_MAP;


Example: Requesting a Memory Block from a Map with No Conditions
================================================================

This code waits indefinitely for a memory block to become
available if all the memory blocks are in use.

.. code-block:: c

  char *block_ptr;

  task_mem_map_alloc_wait(MYMAP, &block_ptr);



Example: Requesting a Memory Block from a Map with a Conditional Time-out
=========================================================================

This code waits a specified amount of time for a memory block to become
available and gives a warning if the memory block does not become available
in the specified time.

.. code-block:: c

  char *block_ptr;

  if (task_mem_map_alloc_wait_timeout(MYMAP, &block_ptr, 5) == RC_OK)) {
    /* utilize memory block */
  } else {
    printf("Memory allocation time-out");
  }



Example: Requesting a Memory Block from a Map with a No Blocking Condition
==========================================================================

This code gives an immediate warning when all memory blocks are in use.

.. code-block:: c

  char *block_ptr;

  if (task_mem_map_alloc(MYMAP, &block_ptr) == RC_OK) {
    /* utilize memory block */
  } else {
    display_warning(); /* and do not allocate memory block*/
  }


Example: Freeing a Memory Block back to a Map
=============================================

This code releases a memory block back when it is no longer needed.

.. code-block:: c

  char *block_ptr;

  task_mem_map_alloc_wait(MYMAP, &block_ptr);
  /* use memory block */
  task_mem_map_free(&block_ptr);



APIs
****

The following Memory Map APIs are provided by :file:`microkernel.h`.

+---------------------------------------------+-----------------------------------+
| Call                                        | Description                       |
+=============================================+===================================+
| :c:func:`task_mem_map_alloc()`              | Requests a block from a memory    |
|                                             | map.                              |
+---------------------------------------------+-----------------------------------+
| :c:func:`task_mem_map_alloc_wait()`         | Waits on a block of memory until  |
|                                             | it is available.                  |
+---------------------------------------------+-----------------------------------+
| :c:func:`task_mem_map_alloc_wait_timeout()` | Waits on a block of memory        |
|                                             | for the period of time            |
|                                             | defined by the time-out           |
|                                             | parameter.                        |
+---------------------------------------------+-----------------------------------+
| :c:func:`task_mem_map_free()`               | Returns a block to a memory map.  |
+---------------------------------------------+-----------------------------------+
| :c:func:`task_mem_map_used_get()`           | Returns the number of used blocks |
|                                             | in a memory map.                  |
+---------------------------------------------+-----------------------------------+