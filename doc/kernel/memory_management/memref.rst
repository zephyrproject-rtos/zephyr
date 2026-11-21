.. _memref:

Memref - Reference counted memory allocation
############################################

The memref API provides reference counted allocations that can have several simultaneous owners.
It is intended for heap allocated objects that cross execution context boundaries, for example
message payloads shared between a producer and multiple consumer threads. The producer allocates a
message, hands it to any number of consumers and releases its own reference. Each consumer that
keeps the message takes its own reference and releases it when done. The last release frees the
block, so no context needs to know how many others still hold it.

.. contents::
   :local:
   :depth: 2

Overview
********

A plain ``malloc`` and ``free`` pair cannot express shared ownership without hand written
accounting at every handoff.

The memref API adds a small control block in front of each allocation. The control block holds an
atomic reference count, an optional destroy callback and the backend the block came from, so the
returned pointer carries its own free path and can cross module boundaries without the receiver
knowing how it was allocated.

Concepts
********

Each allocation starts with a reference count of one.
Any other context that keeps the pointer takes ownership first with :c:func:`memref_ref` and
releases it later with :c:func:`memref_unref`.
When the count reaches zero the optional destroy callback runs and the block is returned to its
backend.

.. mermaid::
   :caption: Lifecycle of an allocation
   :alt: State diagram of an allocation. memref_alloc moves it to the owned state with a
         reference count of one. memref_ref raises the count and memref_unref lowers it while the
         count stays above zero. When memref_unref drops the count to zero the destroy callback
         runs, if set, and the block is returned to its backend.

   stateDiagram-v2
       direction LR
       [*] --> Owned : memref_alloc() / count = 1
       Owned --> Owned : memref_ref() / count + 1
       Owned --> Owned : memref_unref() [count > 1] / count - 1
       Owned --> Releasing : memref_unref() [count == 1] / count = 0
       Releasing --> [*] : destroy callback (if set), backend free()

The control block is private. The pointer returned to the caller addresses only the user payload,
padded so the backend alignment guarantee is kept.

.. mermaid::
   :caption: Byte layout of an allocation on a 32-bit target with 8-byte maximum
             alignment, ``CONFIG_MEMREF_DEBUG`` and ``CONFIG_MEMREF_DESTROY_CB`` disabled
   :alt: Packet diagram of an allocation in bytes. Bytes 0 to 3 hold the reference count and
         bytes 4 to 7 the backend pointer. The user payload starts at byte 8, which is the pointer
         returned by memref_alloc.

   ---
   config:
     packet:
       bitOrder: descending
       bitsPerRow: 24
       bitWidth: 32
   ---
   packet
     0-3: "Reference count"
     4-7: "Backend pointer"
     8-23: "User payload (pointer returned by memref_alloc)"

.. mermaid::
   :caption: Byte layout of an allocation on a 32-bit target with 8-byte maximum
             alignment, ``CONFIG_MEMREF_DEBUG`` disabled and ``CONFIG_MEMREF_DESTROY_CB``
             enabled
   :alt: Packet diagram of an allocation in bytes. Bytes 0 to 3 hold the reference count, bytes
         4 to 7 the destroy callback pointer and bytes 8 to 11 the backend pointer. Bytes 12 to
         15 pad the control block to the maximum alignment. The user payload starts at byte 16,
         which is the pointer returned by memref_alloc.

   ---
   config:
     packet:
       bitOrder: descending
       bitsPerRow: 24
       bitWidth: 32
   ---
   packet
     0-3: "Reference count"
     4-7: "Destroy callback"
     8-11: "Backend pointer"
     12-15: "Alignment padding"
     16-23: "User payload"

The destroy callback pointer is present only with :kconfig:option:`CONFIG_MEMREF_DESTROY_CB`.
:kconfig:option:`CONFIG_MEMREF_DEBUG` adds a magic value after the reference count. The control
block is padded to the maximum alignment, so on such a target it occupies 8 bytes with both options
disabled and 16 bytes with either enabled.

Backends
********

Raw storage comes from a backend with allocate and free callbacks plus an opaque context pointer.
There are two kinds. A variable length backend, :c:struct:`memref_backend`, honors the
requested allocation size.

A fixed block size backend, :c:struct:`memref_block_backend`, wraps a variable length backend whose
underlying allocator serves fixed size blocks decided at compile or initialization time, such as a
:c:struct:`k_mem_slab`.
The distinct type makes calling :c:func:`memref_alloc` on a block backend, or
:c:func:`memref_block_alloc` on a variable length backend, a compile time error.

The backend stays attached to the allocation, so :c:func:`memref_ref` and :c:func:`memref_unref`
only take the user pointer and behave identically regardless of kind. Pointers from different
backends can be passed between modules without tracking their origin. The backend must remain
valid for the lifetime of every allocation made through it, and a backend inherits the
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

A fixed block size backend uses the same ``alloc`` signature, but its ``alloc`` function disregards
the size argument. Each block must be :c:macro:`MEMREF_CTRL_BLOCK_SIZE` bytes larger than the
payload it exposes; :c:macro:`MEMREF_K_MEM_SLAB_DEFINE` accounts for this.

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
       memref_unref(msg); /* Release the sender's reference */
   }

A consumer that keeps the pointer beyond the callback takes a reference with :c:func:`memref_ref`
and releases it with :c:func:`memref_unref` when done. The block is freed once the producer and
every consumer have released their references.

:c:func:`memref_alloc` returns ``NULL`` when the backend is out of memory, when the size is zero or
when the size plus the control block overflows.

:c:macro:`memref_drop` can replace :c:func:`memref_unref` to help find use after free bugs, see
`Debugging`_.

.. mermaid::
   :caption: Ownership handoff between a producer and two consumers
   :alt: Sequence diagram. The producer calls memref_alloc_with_destroy, which takes storage from
         the backend and returns the payload. The producer hands the pointer to two consumers, each
         of which calls memref_ref to claim ownership, and the producer then calls memref_unref to
         relinquish its ownership. Each consumer calls memref_unref when it finishes processing.
         The call that drops the count to zero runs the destroy callback, if set, and returns the
         block to the backend.

   sequenceDiagram
       participant P as Producer
       participant M as memref
       participant B as Backend
       participant C1 as Consumer 1
       participant C2 as Consumer 2

       P->>M: memref_alloc_with_destroy(backend, size, destroy_cb)
       M->>B: alloc(ctx, MEMREF_CTRL_BLOCK_SIZE + size)
       B-->>M: raw block
       M-->>P: payload pointer (count = 1)
       P->>C1: hand off pointer
       C1->>M: memref_ref() (count = 2)
       P->>C2: hand off pointer
       C2->>M: memref_ref() (count = 3)
       P->>M: memref_unref() (count = 2)
       C1->>M: memref_unref() (count = 1)
       C2->>M: memref_unref() (count = 0)
       opt [destroy_cb != NULL]
           M->>M: destroy(payload)
       end
       M->>B: free(ctx, block)

Allocations from a fixed block size backend use :c:func:`memref_block_alloc`, which takes no size
argument. Everything else, including :c:func:`memref_ref`, :c:func:`memref_unref` and
:c:macro:`memref_drop`, is identical.

Destroy callback
----------------

A :c:type:`memref_destroy_cb_t` passed at allocation time runs with the user pointer just before the
block returns to the backend when the last reference is released.
It is passed to :c:func:`memref_alloc_with_destroy` or :c:func:`memref_block_alloc_with_destroy`
instead of :c:func:`memref_alloc` or :c:func:`memref_block_alloc`, and requires
:kconfig:option:`CONFIG_MEMREF_DESTROY_CB`. With the option disabled the ``_with_destroy`` variants
are not defined and such a call fails to build.

Use it to release resources the payload owns, such as a scratchpad or another memref allocation.
Releasing another allocation from the callback lets ownership cascade, so a consumer never needs to
know how the payload was built:

.. code-block:: c

   static void msg_destroy(void *mem)
   {
       struct message *msg = mem;

       k_free(msg->scratchpad);
       memref_unref(msg->payload);
   }

   struct message *msg = memref_alloc_with_destroy(&pool_backend, sizeof(*msg), msg_destroy);

Execution Contexts
******************

:c:func:`memref_ref` and :c:func:`memref_unref` may be called concurrently on the same pointer from
different threads; the count is updated atomically. The destroy callback and the backend free
function run in the context of whichever caller releases the last reference, which may be a
consumer rather than the producer, so :c:func:`memref_unref` inherits the execution context
requirements of the backend and the destroy callback. The ready-made ``k_heap`` and
``k_mem_slab`` backends allocate with ``K_NO_WAIT`` and never block.

Holding a reference is a precondition of :c:func:`memref_unref`, and :c:func:`memref_ref` must not
be used on a pointer whose count already reached zero.

Debugging
*********

:kconfig:option:`CONFIG_MEMREF_DEBUG` stores a magic value in every control block and checks it in
:c:func:`memref_ref` and :c:func:`memref_unref`. Passing a pointer that was not allocated through
the memref API, or one whose last reference was already released, then fails an assertion. The
option depends on :kconfig:option:`CONFIG_ASSERT`.

The check is best effort. The magic value is cleared when the last reference is released, but a
later call reads memory that has already been returned to the backend and may have been reused.

Releasing references with :c:macro:`memref_drop` makes use after free bugs easier to find.
Clearing the variable turns a later use of it into a ``NULL`` dereference that faults at the
offending access, while :kconfig:option:`CONFIG_MEMREF_DEBUG` can miss a block that was already
reused. Copies of the pointer held elsewhere are not cleared.

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
* :kconfig:option:`CONFIG_MEMREF_DEBUG`

API Reference
*************

.. doxygengroup:: memref
