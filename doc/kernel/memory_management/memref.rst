.. _memref:

Shared Memory References
########################

A :dfn:`memref` is a reference counted allocation that can have several simultaneous owners.
It is intended for heap allocated objects that cross execution context boundaries, for example
message payloads shared between a producer and multiple consumer threads. This is extremely useful
for asynchronous message passing, where the producer can allocate a message, send it to multiple
consumers and then release its reference, while the consumers can take their own references and
release them when they are done processing the message. It makes memory allocation and deallocation
much easier to review and maintain, avoids memory leaks, and dangling pointers.

.. contents::
   :local:
   :depth: 2

Overview
********

A plain ``malloc`` and ``free`` pair cannot express shared ownership.
When several contexts need to hold the same block, the last owner to finish must release it, which
otherwise requires hand written accounting at every handoff or assumptions about the lifetime of the
block.

The memref API in :zephyr_file:`include/zephyr/sys/memref.h` adds a small control block in front of
each allocation. The control block holds an atomic reference count, an optional teardown callback
and the backend the block came from. The implementation is in :zephyr_file:`lib/memref/memref.c` and
is enabled with :kconfig:option:`CONFIG_MEMREF`.

Concepts
********

Each allocation starts with a reference count of one.
The initial owner releases ownership with :c:func:`memref_unref`.
Any other context that keeps the pointer takes ownership first with :c:func:`memref_ref` and
releases it later with :c:func:`memref_unref`.
When the count reaches zero the optional teardown callback runs and the block is returned to its
backend.

The control block is private. The pointer returned to the caller addresses only the user payload,
padded so the backend alignment guarantee is kept. Size addition overflow on allocation and element
count overflow on calloc are rejected with ``NULL``.

Backends
********

Raw storage comes from a :c:struct:`memref_backend` with allocate and free callbacks plus an opaque
context pointer:

.. code-block:: c

   struct memref_backend {
       memref_alloc_func_t alloc;
       memref_free_func_t free;
       void *ctx;
   };

The backend stays attached to the allocation, so :c:func:`memref_ref` and :c:func:`memref_unref`
only take the user pointer. Pointers from different backends can be passed between modules without
tracking their origin. The backend must remain valid for the lifetime of every allocation made
through it, and a memref inherits the limitations of its backend.

Defining a backend
==================

Wrap an existing allocator and publish it with :c:macro:`MEMREF_BACKEND_DEFINE`:

.. code-block:: c

   #include <zephyr/kernel.h>
   #include <zephyr/sys/memref.h>

   static void *heap_alloc(void *ctx, size_t size)
   {
       return k_heap_alloc((struct k_heap *)ctx, size, K_NO_WAIT);
   }

   static void heap_free(void *ctx, void *mem)
   {
       k_heap_free((struct k_heap *)ctx, mem);
   }

   K_HEAP_DEFINE(pool, 4096);
   MEMREF_BACKEND_DEFINE(pool_backend, heap_alloc, heap_free, &pool);

The same pattern works on top of :c:func:`k_malloc` and :c:func:`k_free`, or on top of a memory slab
with :c:func:`k_mem_slab_alloc` and :c:func:`k_mem_slab_free`.
See ``tests/lib/memref/src/`` for complete examples.

Usage
*****

Allocating and releasing
========================

A receiver that keeps the pointer beyond the call takes a reference:

.. code-block:: c

   void broadcast_to_consumers(struct message *msg) {
       for (int i = 0; i < NUM_CONSUMERS; i++) {
           consumer[i].call_back(msg); // consumer can choose to take shared ownership before return
       }
   }

   ... {
       struct message *msg = memref_alloc(backend, sizeof(*msg), NULL);
       ....
       broadcast_to_consumers(msg);
       memref_unref(msg); // Release the sender's reference
   }

Each consumer then calls :c:func:`memref_ref` if they want to keep the reference and process it
later.
If a consumer makes a ownership claim (calls :c:func:`memref_ref`) it must eventually call
:c:func:`memref_unref` once it is done with the block.
The block is reclaimed once the sender and every consumer have released it.

Teardown callback
-----------------

A :c:type:`memref_free_cb_t` passed at allocation time runs with the user pointer just before the
block returns to the backend when the last reference is dropped.
Use it to release resources tied to the payload:

.. code-block:: c

   static void msg_cleanup(void *mem)
   {
       struct message *msg = mem;

       k_free(msg->extra);
   }

   struct message *msg = memref_alloc(&pool_backend, sizeof(*msg), msg_cleanup);

Execution Contexts
******************

:c:func:`memref_ref` and :c:func:`memref_unref` inherit the execution context of the backend
callbacks and the teardown callback.
Do not call them from ISR unless the backend and the teardown callback of that allocation are
documented as ISR safe.
Holding a reference is a precondition of :c:func:`memref_unref`, and :c:func:`memref_ref` must not
be used on a pointer whose count already reached zero.

Configuration
*************

Related configuration options:

* :kconfig:option:`CONFIG_MEMREF`

API Reference
*************

.. doxygengroup:: memref
