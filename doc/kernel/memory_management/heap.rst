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
:c:macro:`K_HEAP_DEFINE` macro.  This creates a static :c:struct:`k_heap` variable
with a given name that manages a memory region of the
specified size.

Heaps can also be created to manage arbitrary regions of
application-controlled memory using :c:func:`k_heap_init`.

Allocating Memory
=================

Memory can be allocated from a heap using :c:func:`k_heap_alloc`,
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

Memory allocated with :c:func:`k_heap_alloc` must be released using
:c:func:`k_heap_free`.  Similar to standard C ``free()``, the pointer
provided must be either a ``NULL`` value or a pointer previously
returned by :c:func:`k_heap_alloc` for the same heap.  Freeing a
``NULL`` value is defined to have no effect.

Low Level Heap Allocator
************************

The underlying implementation of the :c:struct:`k_heap`
abstraction is provided a data structure named :c:struct:`sys_heap`.  This
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
:c:struct:`sys_heap` structure itself.

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
resistance.  This :kconfig:option:`CONFIG_SYS_HEAP_ALLOC_LOOPS` value may be
chosen by the user at build time, and defaults to a value of 3.

Multi-Heap Wrapper Utility
**************************

The ``sys_heap`` utility requires that all managed memory be in a
single contiguous block.  It is common for complicated microcontroller
applications to have more complicated memory setups that they still
want to manage dynamically as a "heap".  For example, the memory might
exist as separate discontiguous regions, different areas may have
different cache, performance or power behavior, peripheral devices may
only be able to perform DMA to certain regions, etc...

For those situations, Zephyr provides a ``sys_multi_heap`` utility.
Effectively this is a simple wrapper around a set of one or more
``sys_heap`` objects.  It should be initialized after its child heaps
via :c:func:`sys_multi_heap_init`, after which each heap can be added
to the managed set via :c:func:`sys_multi_heap_add_heap`.  No
destruction utility is provided; just as for ``sys_heap``,
applications that want to destroy a multi heap should simply ensure
all allocated blocks are freed (or at least will never be used again)
and repurpose the underlying memory for another usage.

It has a single pair of allocation entry points,
:c:func:`sys_multi_heap_alloc` and
:c:func:`sys_multi_heap_aligned_alloc`.  These behave identically to
the ``sys_heap`` functions with similar names, except that they also
accept an opaque "configuration" parameter.  This pointer is
uninspected by the multi heap code itself; instead it is passed to a
callback function provided at initialization time.  This
application-provided callback is responsible for doing the underlying
allocation from one of the managed heaps, and may use the
configuration parameter in any way it likes to make that decision.

For modifying the size of an allocated buffer (whether shrinking
or enlarging it), you can use the
:c:func:`sys_multi_heap_realloc` and
:c:func:`sys_multi_heap_aligned_realloc` APIs.  If the buffer cannot be
enlarged on the heap where it currently resides,
any of the eligible heaps specified by the configuration parameter may be used.

When unused, a multi heap may be freed via
:c:func:`sys_multi_heap_free`.  The application does not need to pass
a configuration parameter.  Memory allocated from any of the managed
``sys_heap`` objects may be freed with in the same way.

System Heap
***********

The :dfn:`system heap` is a predefined memory allocator that allows
threads to dynamically allocate memory from a common memory region in
a :c:func:`malloc`-like manner.

Only a single system heap is defined. Unlike other heaps or memory
pools, the system heap cannot be directly referenced using its
memory address.

The size of the system heap is configurable to arbitrary sizes,
subject to space availability.

A thread can dynamically allocate a chunk of heap memory by calling
:c:func:`k_malloc`. The address of the allocated chunk is
guaranteed to be aligned on a multiple of pointer sizes. If a suitable
chunk of heap memory cannot be found ``NULL`` is returned.

When the thread is finished with a chunk of heap memory it can release
the chunk back to the system heap by calling :c:func:`k_free`.

Defining the Heap Memory Pool
=============================

The size of the heap memory pool is specified using the
:kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE` configuration option.

By default, the heap memory pool size is zero bytes. This value instructs
the kernel not to define the heap memory pool object. The maximum size is limited
by the amount of available memory in the system. The project build will fail in
the link stage if the size specified can not be supported.

In addition, each subsystem (board, driver, library, etc) can set a custom
requirement by defining a Kconfig option with the prefix
``HEAP_MEM_POOL_ADD_SIZE_`` (this value is in bytes). If multiple subsystems
specify custom values, the sum of these will be used as the minimum requirement.
If the application tries to set a value that's less than the minimum value, this
will be ignored and the minimum value will be used instead.

To force a smaller than minimum value to be used, the application may enable the
:kconfig:option:`CONFIG_HEAP_MEM_POOL_IGNORE_MIN` option. This can be useful
when optimizing the heap size and the minimum requirement can be more accurately
determined for a specific application.

Allocating Memory
=================

A chunk of heap memory is allocated by calling :c:func:`k_malloc`.

The following code allocates a 200 byte chunk of heap memory, then fills it
with zeros. A warning is issued if a suitable chunk is not obtained.

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

A chunk of heap memory is released by calling :c:func:`k_free`.

The following code allocates a 75 byte chunk of memory, then releases it
once it is no longer needed.

.. code-block:: c

    char *mem_ptr;

    mem_ptr = k_malloc(75);
    ... /* use memory block */
    k_free(mem_ptr);

Suggested Uses
==============

Use the heap memory pool to dynamically allocate memory in a
:c:func:`malloc`-like manner.

Configuration Options
=====================

Related configuration options:

* :kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE`

API Reference
=============

.. doxygengroup:: heap_apis

.. doxygengroup:: low_level_heap_allocator

.. doxygengroup:: multi_heap_wrapper

Heap listener
*************

.. doxygengroup:: heap_listener_apis
