.. _heap_v2:

Heap Memory Pool
################

The :dfn:`heap memory pool` is a predefined memory pool object that allows
threads to dynamically allocate memory from a common memory region
in a :cpp:func:`malloc()`-like manner.

.. contents::
    :local:
    :depth: 2

Concepts
********

Only a single heap memory pool can be defined. Unlike other memory pools,
the heap memory pool cannot be directly referenced using its memory address.

The size of the heap memory pool is configurable. The following sizes
are supported: 256 bytes, 1024 bytes, 4096 bytes, and 16384 bytes.

A thread can dynamically allocate a chunk of heap memory by calling
:cpp:func:`k_malloc()`. The address of the allocated chunk is guaranteed
to be aligned on a multiple of 4 bytes. If a suitable chunk of heap memory
cannot be found :c:macro:`NULL` is returned.

When the thread is finished with a chunk of heap memory it can release
the chunk back to the heap memory pool by calling :cpp:func:`k_free()`.

Internal Operation
==================

The heap memory pool defines a single maximum size block that contains
the entire heap; that is, a single block of 256, 1024, 4096, or 16384 bytes.
The heap memory pool also defines a minimum block size of 64 bytes.
Consequently, the maximum number of blocks of each size that the heap
memory pool can support is shown in the following table.

+-------+---------+----------+-----------+-----------+------------+
| heap  | 64 byte | 256 byte | 1024 byte | 4096 byte | 16384 byte |
| size  | blocks  | blocks   | blocks    | blocks    | blocks     |
+=======+=========+==========+===========+===========+============+
| 256   | 4       | 1        | 0         | 0         | 0          |
+-------+---------+----------+-----------+-----------+------------+
| 1024  | 16      | 4        | 1         | 0         | 0          |
+-------+---------+----------+-----------+-----------+------------+
| 4096  | 64      | 16       | 4         | 1         | 0          |
+-------+---------+----------+-----------+-----------+------------+
| 16384 | 256     | 64       | 16        | 4         | 1          |
+-------+---------+----------+-----------+-----------+------------+

.. note::
    The number of blocks of a given size that can be allocated
    simultaneously is typically smaller than the value shown in the table.
    For example, each allocation of a 256 byte block from a 1024 byte
    heap reduces the number of 64 byte blocks available for allocation
    by 4. Fragmentation of the memory pool's buffer can also further
    reduce the availability of blocks.

The kernel uses the first 16 bytes of any memory block allocated
from the heap memory pool to save the block descriptor information
it needs to later free the block. Consequently, an application's request
for an N byte chunk of heap memory requires a block that is at least
(N+16) bytes long.

Implementation
**************

Defining the Heap Memory Pool
=============================

The size of the heap memory pool is specified using the
:option:`CONFIG_HEAP_MEM_POOL_SIZE` configuration option.

By default, the heap memory pool size is zero bytes. This value instructs
the kernel not to define the heap memory pool object.

Allocating Memory
=================

A chunk of heap memory is allocated by calling :cpp:func:`k_malloc()`.

The following code allocates a 200 byte chunk of heap memory, then fills it
with zeros. A warning is issued if a suitable chunk is not obtained.

Note that the application will actually allocate a 256 byte memory block,
since that is the closest matching size supported by the heap memory pool.

.. code-block:: c

    char *mem_ptr;

    mem_ptr = k_malloc(200);
    if (mem_ptr != NULL)) {
        memset(mem_ptr, 0, 200);
	...
    } else {
        printf("Memory not allocated");
    }

Releasing Memory
================

A chunk of heap memory is released by calling :cpp:func:`k_free()`.

The following code allocates a 75 byte chunk of memory, then releases it
once it is no longer needed. (A 256 byte memory block from the heap memory
pool is actually used to satisfy the request.)

.. code-block:: c

    char *mem_ptr;

    mem_ptr = k_malloc(75);
    ... /* use memory block */
    k_free(mem_ptr);

Suggested Uses
**************

Use the heap memory pool to dynamically allocate memory in a
:cpp:func:`malloc()`-like manner.

Configuration Options
*********************

Related configuration options:

* :option:`CONFIG_HEAP_MEM_POOL_SIZE`

API Reference
*************

.. doxygengroup:: heap_apis
   :project: Zephyr
