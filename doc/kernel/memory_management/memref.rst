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

The memref API  adds a small control block in front of each allocation. The control block holds an
reference count, an optional destroy callback and the backend the block came from, so the returned
pointer carries its own free path and can cross module boundaries without the receiver knowing how
it was allocated.

Concepts
********

Each allocation starts with a reference count of one.
Any other context that keeps the pointer takes ownership first with :c:func:`memref_ref` and
releases it later with :c:func:`memref_unref`.
When the count reaches zero the optional destroy callback runs and the block is returned to its
backend.

The control block is private. The pointer returned to the caller addresses only the user payload,
padded so the backend alignment guarantee is kept. Size addition overflow on allocation is rejected
with ``NULL``.

Backends
********

Raw storage comes from a backend with allocate and free callbacks plus an opaque context pointer.
There are two flavours. A variable length backend, :c:struct:`memref_backend`, honours the
requested allocation size.

A fixed block size backend, :c:struct:`memref_block_backend`, wraps a variable length backend whose
underlying allocator serves fixed size blocks decided at compile or initialization time, such as a
:c:struct:`k_mem_slab`.
The distinct type makes calling :c:func:`memref_alloc` on a block backend, or
:c:func:`memref_block_alloc` on a variable length backend, a compile time error.

The backend stays attached to the allocation, so :c:func:`memref_ref` and :c:func:`memref_unref`
only take the user pointer and behave identically regardless of flavour. Pointers from different
backends can be passed between modules without tracking their origin. The backend must remain
valid for the lifetime of every allocation made through it, and a memref backend inherits the
limitations of its memory allocator.

Defining a backend
==================

For the common allocators, define a backend with one of the ready-made macros:

.. code-block:: c

   #include <zephyr/sys/memref.h>

   /* Variable length: defines the heap and the matching backend */
   MEMREF_K_HEAP_DEFINE(pool_backend, 4096);

   /* Fixed block size: defines the slab and the matching block backend. The
    * slab block size accounts for the control block; 128 is the user payload.
    */
   MEMREF_K_MEM_SLAB_DEFINE(slab_backend, 128, 10, sizeof(void *));

To wrap any other allocator, publish a backend with :c:macro:`MEMREF_BACKEND_DEFINE` or
:c:macro:`MEMREF_BLOCK_BACKEND_DEFINE`:

.. code-block:: c

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

A fixed block size backend provides the same ``alloc`` signature, but its ``alloc`` function
disregards the size argument as the block size is fixed at compile or initialization time.

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
       struct message *msg = memref_alloc(backend, sizeof(*msg));
       ....
       broadcast_to_consumers(msg);
       memref_unref(msg); // Release the sender's reference
   }

Each consumer then calls :c:func:`memref_ref` if they want to keep the reference and process it
later.
If a consumer makes a ownership claim (calls :c:func:`memref_ref`) it must eventually call
:c:func:`memref_unref` once it is done with the block.
The block is reclaimed once the sender and every consumer have released it.

Allocations from a fixed block size backend use :c:func:`memref_block_alloc`, which takes no size
argument. Everything else, including :c:func:`memref_ref` and :c:func:`memref_unref`, is identical.

Destroy callback
----------------

A :c:type:`memref_destroy_cb_t` passed at allocation time runs with the user pointer just before the
block returns to the backend when the last reference is dropped.
It is passed to :c:func:`memref_alloc_with_destroy` or :c:func:`memref_block_alloc_with_destroy`
instead of :c:func:`memref_alloc` or :c:func:`memref_block_alloc`, and requires
:kconfig:option:`CONFIG_MEMREF_DESTROY_CB`.

Use the callback to release resources tied to the payload:

.. code-block:: c

   static void msg_destroy(void *mem)
   {
       struct message *msg = mem;

       k_free(msg->extra);
   }

   struct message *msg = memref_alloc_with_destroy(&pool_backend, sizeof(*msg), msg_destroy);

Execution Contexts
******************

:c:func:`memref_ref` and :c:func:`memref_unref` inherit the execution context of the backend
callbacks and the destroy callback.
Do not call them from ISR unless the backend and the destroy callback of that allocation are
documented as ISR safe.
Holding a reference is a precondition of :c:func:`memref_unref`, and :c:func:`memref_ref` must not
be used on a pointer whose count already reached zero.

Sample
******

The :zephyr:code-sample:`memref` sample (:zephyr_file:`samples/data_structures/memref`) demonstrates
the API on a ``k_heap`` backend. A producer allocates a message, shares ownership with three
consumer threads through message queues, and releases its own reference, while each consumer
releases its reference after processing.

Configuration
*************

Related configuration options:

* :kconfig:option:`CONFIG_MEMREF`
* :kconfig:option:`CONFIG_MEMREF_DESTROY_CB`

API Reference
*************

.. doxygengroup:: memref
