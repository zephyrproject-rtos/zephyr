.. _heap_v2:

Memory Heaps
############

Zephyr provides a collection of utilities that allow threads to
dynamically allocate memory.

Synchronized Heap Allocator
***************************

Creating a Heap
===============

The simplest way to define a heap is statically, with the
:c:macro:`K_HEAP_DEFINE` macro.  This creates a static :c:type:`k_heap` variable
with a given name that manages a memory region of the
specified size.

Heaps can also be created to manage arbitrary regions of
application-controlled memory using :cpp:func:`k_heap_init()`.

Allocating Memory
=================

Memory can be allocated from a heap using :cpp:func:`k_heap_alloc()`,
passing it the address of the heap object and the number of bytes
desired.  This functions similarly to standard C ``malloc()``,
returning a NULL pointer on an allocation failure.

The heap supports blocking operation, allowing threads to go to sleep
until memory is available.  The final argument is a
:c:type:`k_timeout_t` timeout value indicating how long the thread may
sleep before returning, or else one of the constant timeout values
:c:macro:`K_NO_WAIT` or :c:macro:`K_FOREVER`.

Releasing Memory
================

Memory allocated with :cpp:func:`k_heap_alloc()` must be released using
:cpp:func:`k_heap_free()`.  Similar to stanard C ``free()``, the pointer
provided must be either a ``NULL`` value or a pointer previously
returned by :cpp:func:`k_heap_alloc()` for the same heap.  Freeing a
``NULL`` value is defined to have no effect.

Low Level Heap Allocator
************************

The underlying implementation of the :c:type:`k_heap`
abstraction is provided a data structure named :c:type:`sys_heap`.  This
implements exactly the same allocation semantics, but
provides no kernel synchronization tools.  It is available for
applications that want to manage their own blocks of memory in
contexts (for example, userspace) where synchronization is unavailable
or more complicated.  Unlike ``k_heap``, all calls to any ``sys_heap``
functions on a single heap must be serialized by the caller.
Simultaneous use from separate threads is disallowed.

Implementation
==============

Internally, the ``sys_heap`` memory block is partitioned into "chunks"
of 8 bytes.  All allocations are made out of a contiguous region of
chunks.  The first chunk of every allocation or unused block is
prefixed by a chunk header that stores the length of the chunk, the
length of the next lower ("left") chunk in physical memory, a bit
indicating whether the chunk is in use, and chunk-indexed link
pointers to the previous and next chunk in a "free list" to which
unused chunks are added.

The heap code takes reasonable care to avoid fragmentation.  Free
block lists are stored in "buckets" by their size, each bucket storing
blocks within one power of two (i.e. a bucket for blocks of 3-4
chunks, another for 5-8, 9-16, etc...) this allows new allocations to
be made from the smallest/most-fragmented blocks available.  Also, as
allocations are freed and added to the heap, they are automatically
combined with adjacent free blocks to prevent fragmentation.

All metadata is stored at the beginning of the contiguous block of
heap memory, including the variable-length list of bucket list heads
(which depend on heap size).  The only external memory required is the
:c:type:`sys_heap` structure itself.

The ``sys_heap`` functions are unsynchronized.  Care must be taken by
any users to prevent concurrent access.  Only one context may be
inside one of the API functions at a time.

The heap code takes care to present high performance and reliable
latency.  All ``sys_heap`` API functions are guaranteed to complete
within constant time.  On typical architectures, they will all
complete within 1-200 cycles.  One complexity is that the search of
the minimum bucket size for an allocation (the set of free blocks that
"might fit") has a compile-time upper bound of iterations to prevent
unbounded list searches, at the expense of some fragmentation
resistance.  This :c:option:`CONFIG_SYS_HEAP_ALLOC_LOOPS` value may be
chosen by the user at build time, and defaults to a value of 3.

System Heap
***********

The :dfn:`system heap` is a predefined memory allocator that allows
threads to dynamically allocate memory from a common memory region in
a :cpp:func:`malloc()`-like manner.

Only a single system heap is be defined. Unlike other heaps or memory
pools, the system heap cannot be directly referenced using its
memory address.

The size of the system heap is configurable to arbitrary sizes,
subject to space availability.

A thread can dynamically allocate a chunk of heap memory by calling
:cpp:func:`k_malloc()`. The address of the allocated chunk is
guaranteed to be aligned on a multiple of pointer sizes. If a suitable
chunk of heap memory cannot be found :c:macro:`NULL` is returned.

When the thread is finished with a chunk of heap memory it can release
the chunk back to the system heap by calling :cpp:func:`k_free()`.

Defining the Heap Memory Pool
=============================

The size of the heap memory pool is specified using the
:option:`CONFIG_HEAP_MEM_POOL_SIZE` configuration option.

By default, the heap memory pool size is zero bytes. This value instructs
the kernel not to define the heap memory pool object. The maximum size is limited
by the amount of available memory in the system. The project build will fail in
the link stage if the size specified can not be supported.

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
==============

Use the heap memory pool to dynamically allocate memory in a
:cpp:func:`malloc()`-like manner.

Configuration Options
=====================

Related configuration options:

* :option:`CONFIG_HEAP_MEM_POOL_SIZE`

API Reference
=============

.. doxygengroup:: heap_apis
   :project: Zephyr
