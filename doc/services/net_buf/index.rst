.. _net_buf_interface:

Network Buffers
###############

.. contents::
    :local:
    :depth: 2


Overview
********

Network buffers are a core concept of how the networking stack
(as well as the Bluetooth stack) pass data around. The API for them is
defined in :zephyr_file:`include/zephyr/net_buf.h`.

Buffer Layout
*************

The data of a buffer is kept in a block of storage whose size is fixed
when the buffer is allocated. The data does not have to start at the
beginning of the storage: there can be free space before it, called the
headroom, and after it, called the tailroom.

.. mermaid::
   :caption: Layout of a network buffer
   :alt: The storage of a buffer from __buf to __buf + size, divided into
         headroom, len and tailroom. The data pointer marks the boundary
         between headroom and len, and data + len the boundary between len
         and tailroom. The size arrow spans all three.

   block-beta
     columns 12
     p0["__buf"] space:2 p1["data"] space:4 p2["data + len"] space:2 p3["__buf + size"]
     headroom["headroom"]:3 len["len"]:5 tailroom["tailroom"]:4
     size<["size"]>(x):12

     classDef ptr fill:none,stroke:none
     classDef free stroke-dasharray:5,fill-opacity:0.3
     class p0,p1,p2,p3 ptr
     class headroom,tailroom free

Four fields of the buffer describe this layout:

``data``
  Points to the start of the data.

``len``
  Length of the data behind the ``data`` pointer.

``size``
  Size of the storage, headroom and tailroom included.

``__buf``
  Start of the storage. Not to be accessed directly: use the ``data``
  pointer instead.

The headroom is returned by :c:func:`net_buf_headroom`, the tailroom by
:c:func:`net_buf_tailroom`, and :c:func:`net_buf_tail` returns
``data + len``, where the next byte added to the buffer goes. The room
left for more data is the tailroom, not ``size`` minus ``len``:
``size`` counts the headroom as well.

Creating buffers
****************

Network buffers are created by first defining a pool of them, with the
macro of the pool's type (see `Pool Types`_). A pool of fixed-size
buffers is defined with:

.. code-block:: c

   NET_BUF_POOL_FIXED_DEFINE(pool_name, buf_count, buf_size, user_data_size, NULL);

The pool is a static variable, so if it's needed to be exported to
another module a separate pointer is needed.

Once the pool has been defined, buffers can be allocated from it with:

.. code-block:: c

   buf = net_buf_alloc(&pool_name, timeout);

There is no explicit initialization function for the pool or its
buffers, rather this is done implicitly as :c:func:`net_buf_alloc` gets
called.

If there is a need to reserve space in the buffer for protocol headers
to be prepended later, it's possible to reserve this headroom with:

.. code-block:: c

   net_buf_reserve(buf, headroom);

In addition to actual protocol data and generic parsing context, network
buffers may also contain protocol-specific context, known as user data.
The user data size is a property of the pool: every buffer of the pool
has ``user_data_size`` bytes of it, returned by
:c:func:`net_buf_user_data`.

The last argument of the pool definition is an optional destroy
callback. When the last reference to a buffer is released, the callback
is called instead of returning the buffer to the pool, and it must
eventually call :c:func:`net_buf_destroy` to return it.

The buffers have native support for being passed through k_fifo kernel
objects. Use :c:func:`k_fifo_put` and :c:func:`k_fifo_get` to pass buffer
from one thread to another.

Buffers can also be kept in a :c:type:`sys_slist_t` through their
``node`` field. :c:func:`net_buf_slist_put` and :c:func:`net_buf_slist_get`
append and remove a buffer under a spinlock, so that a list can be shared
between threads and interrupt handlers as long as every access to it goes
through these two functions. Use them instead of
:c:func:`sys_slist_append` and :c:func:`sys_slist_get` for such a list.
A buffer has a single ``node`` field, so it can be in only one FIFO or
list at a time, however many references to it exist.

Pool Types
==========

The pool type determines where the data of its buffers comes from:

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - Pool
     - Data storage
     - Data allocation
   * - :c:macro:`NET_BUF_POOL_FIXED_DEFINE`
     - A static array with one chunk of ``data_size`` bytes per buffer.
     - Always the full chunk. Never waits.
   * - :c:macro:`NET_BUF_POOL_VAR_DEFINE`
     - A heap of ``data_size`` bytes shared by the buffers of the pool.
     - The size requested for each buffer. Waits for heap memory until the
       allocation timeout expires.
   * - :c:macro:`NET_BUF_POOL_VAR_ALIGN_DEFINE`
     - Same as :c:macro:`NET_BUF_POOL_VAR_DEFINE`.
     - Same as :c:macro:`NET_BUF_POOL_VAR_DEFINE`, with the start of the
       data aligned to ``align`` and the allocation rounded up to a
       multiple of it. A request for less than ``align`` bytes fails.
   * - :c:macro:`NET_BUF_POOL_HEAP_DEFINE`
     - The system heap, through :c:func:`k_malloc`. Needs
       :kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE`.
     - The size requested for each buffer. Never waits: fails at once when
       the heap is exhausted.

In every pool type the buffers themselves come from a fixed array of
``buf_count`` entries, and an allocation waits for a free one until its
timeout expires. The data is allocated after that, within what is left
of the same timeout. An allocation from a heap pool can therefore return
``NULL`` even with :c:macro:`K_FOREVER`. Each allocation from a variable
or heap pool also uses a few bytes of the heap for bookkeeping, which
the size of a variable pool's heap has to allow for.

:c:func:`net_buf_alloc` allocates a buffer with the fixed data size of
its pool and is meant for fixed pools: from a variable or heap pool it
returns a buffer without data storage. Use :c:func:`net_buf_alloc_len`
with those. :c:func:`net_buf_alloc_with_data` works with every pool type:
the buffer then uses storage given by the caller, and the pool allocates
no data for it. The buffer starts with all of that storage as its data,
so it has no tailroom, and releasing the buffer does not release the
storage: the caller keeps it valid for as long as the buffer exists.

Allocation Timeouts
===================

The timeout of an allocation is how long it may wait for a free buffer
and, in a variable pool, for memory for the data. An interrupt handler
cannot wait and has to use :c:macro:`K_NO_WAIT`.

A thread that waits with :c:macro:`K_FOREVER` on a pool whose buffers
only it frees, directly or through work that it processes itself, never
wakes up. Use a finite timeout in such a context and handle the ``NULL``
return.

Common Operations
*****************

The network buffer API provides some useful helpers for encoding and
decoding data in the buffers. To fully understand these helpers it's
good to understand the basic names of operations used with them:

.. mermaid::
   :caption: Where the four operations change the data of a buffer
   :alt: The buffer as headroom, len and tailroom. Push moves the start of
         the data into the headroom and pull moves it towards the end.
         Add moves the end of the data into the tailroom and remove moves
         it towards the start.

   block-beta
     columns 24
     space:2 push<["push"]>(left):4 space:10 add<["add"]>(right):4 space:4
     headroom["headroom"]:6 len["len"]:10 tailroom["tailroom"]:8
     space:6 pull<["pull"]>(right):4 space:2 remove<["remove"]>(left):4 space:8

     classDef free stroke-dasharray:5,fill-opacity:0.3
     class headroom,tailroom free

.. list-table::
   :header-rows: 1
   :widths: 12 18 18 22 30

   * - Operation
     - Works at
     - Changes
     - Needs
     - Typical use
   * - Add
     - End of the data
     - ``len``
     - Enough tailroom
     - Encoding a message from its start
   * - Remove
     - End of the data
     - ``len``
     - Enough data
     - Decoding a trailer, such as a checksum
   * - Push
     - Start of the data
     - ``data`` and ``len``
     - Enough headroom
     - Prepending the header of a lower layer
   * - Pull
     - Start of the data
     - ``data`` and ``len``
     - Enough data
     - Decoding a header

Each operation has a function per type of data, for instance for Add:

``net_buf_add_mem()``
  Copies a memory area.

``net_buf_add_u8()``
  Adds one byte.

``net_buf_add_le16()``, ``net_buf_add_be16()``
  Add an integer in little-endian or big-endian byte order. Both byte
  orders exist for 16, 24, 32, 40, 48 and 64 bits.

Push, Pull and Remove have the same set, named ``net_buf_push_*()``,
``net_buf_pull_*()`` and ``net_buf_remove_*()``. The byte and integer
variants of Pull and Remove return the value they pulled or removed.
:c:func:`net_buf_add`, :c:func:`net_buf_push` and :c:func:`net_buf_pull`
move the boundary by a length without copying anything, and return a
pointer into the buffer:

.. code-block:: c

   struct foo_hdr *hdr = net_buf_push(buf, sizeof(*hdr));

   hdr->opcode = FOO_OP_WRITE;
   hdr->len = sys_cpu_to_le16(len);

Simple Buffers
**************

:c:struct:`net_buf` is built around :c:struct:`net_buf_simple`, which has
only the four fields of the buffer layout. What :c:struct:`net_buf` adds
is what a buffer needs to be passed between contexts: the reference
count, the pool that the buffer returns to, the link for FIFOs and lists,
the fragment chain and the user data.

A :c:struct:`net_buf_simple` is enough for a buffer that one function or
object uses on its own, such as a short message encoded on the stack.
:c:macro:`NET_BUF_SIMPLE_DEFINE` defines one together with its storage,
and :c:macro:`NET_BUF_SIMPLE_DEFINE_STATIC` a static one. Each of the
add, remove, push and pull functions has a ``net_buf_simple_`` variant:

.. code-block:: c

   NET_BUF_SIMPLE_DEFINE(msg, 8);

   net_buf_simple_add_u8(&msg, opcode);
   net_buf_simple_add_le16(&msg, handle);

A parser that hands a buffer to code that pulls data from it can save the
position of the data first with :c:func:`net_buf_simple_save` and return
to it afterwards with :c:func:`net_buf_simple_restore`:

.. code-block:: c

   NET_BUF_SIMPLE_DEFINE(msg, 32);
   struct net_buf_simple_state state;

   net_buf_simple_add_mem(&msg, data, len);

   net_buf_simple_save(&msg, &state);
   err = parse_header(&msg);
   net_buf_simple_restore(&msg, &state);

The saved state is the position of the data, not its content: data
written into the buffer in between is not undone.

Fragments
*********

Data that does not fit in one buffer, or that is put together from
pieces, is kept in a chain of buffers linked through their ``frags``
field. The first buffer of the chain is its head.

.. mermaid::
   :caption: A chain of three buffers
   :alt: Three buffers in a row, each with its own headroom, data and
         tailroom. The frags field of the head points to the second
         buffer, the frags field of the second buffer to the third, and
         the frags field of the third is NULL.

   block-beta
     columns 16
     t0["head"]:4 space t1["fragment"]:4 space t2["fragment"]:4 space:2
     h0[" "] l0["len"]:2 r0[" "] a0<["frags"]>(right) h1[" "] l1["len"]:3 a1<["frags"]>(right) l2["len"]:2 r2[" "]:2 a2<["frags"]>(right) n["NULL"]

     classDef ptr fill:none,stroke:none
     classDef free stroke-dasharray:5,fill-opacity:0.3
     class t0,t1,t2,n ptr
     class h0,r0,h1,r2 free

Each buffer of the chain has its own headroom and tailroom, and its
``len`` counts only its own data. :c:func:`net_buf_frags_len` returns the
length of the data in the whole chain.

:c:func:`net_buf_frag_add` adds a fragment to the end of a chain,
:c:func:`net_buf_frag_insert` inserts one after a given buffer, and
:c:func:`net_buf_frag_del` removes one and releases the chain's
reference to it. Who owns the buffers of a chain is described in
`Ownership`_.

These functions work on the data of a whole chain:

:c:func:`net_buf_linearize`
  Copies data from an offset in the chain into a contiguous buffer. It
  returns the number of bytes copied, which is less than requested when
  the chain or the destination is shorter.

:c:func:`net_buf_append_bytes`
  Adds data to the end of the chain and allocates more fragments when the
  last one is full, from the pool of the buffer it is given or through an
  allocator callback. The timeout applies to each fragment allocation.
  It returns the number of bytes added, which is less than requested when
  a fragment could not be allocated.

:c:func:`net_buf_data_match`
  Compares data with the content of the chain from an offset, and returns
  the number of bytes that match before the first difference.

:c:func:`net_buf_skip`
  Removes data from the start of the chain, releases the fragments it
  empties, and returns what is left of the chain.

Ownership
*********

Network buffers are reference counted. A newly allocated buffer has one
reference, and the buffer returns to its pool when the last reference is
released. Each reference has exactly one owner. The owner either moves
the reference to a new owner, such as a FIFO, a list or a function that
takes ownership of the buffer, or releases it. After the move the
previous owner must no longer use the buffer, unless it holds another
reference of its own, acquired with :c:func:`net_buf_ref`.

Two helpers operate on the pointer that holds a reference, so that the
previous owner is left with ``NULL`` instead of a pointer to a buffer it
no longer owns. :c:func:`net_buf_take` moves the reference out of the
pointer, and :c:func:`net_buf_drop` releases it. :c:func:`net_buf_drop`
is meant for a pointer that outlives the release, such as a structure
member, or one that may already be ``NULL``. A local pointer that holds a
reference as it goes out of scope only needs :c:func:`net_buf_unref`.

A fragment chain is owned through its head. Each buffer in the chain
owns a reference to the next fragment and releases it when the buffer
itself is freed, so releasing the last reference to the head releases
the chain up to the first fragment that is still referenced elsewhere.
:c:func:`net_buf_frag_insert` takes ownership of the fragment it is
given, and so does :c:func:`net_buf_frag_add` when the head is not
``NULL``. With a ``NULL`` head, :c:func:`net_buf_frag_add` returns the
fragment with a new reference and the caller keeps its own.

A function or callback that is given a buffer does one of the following
with the reference, and its documentation should say which:

Borrows
  The function may use the buffer until it returns, but may neither keep
  nor move it. To keep a borrowed buffer, it acquires a reference of its
  own with :c:func:`net_buf_ref` first.

Takes ownership
  The reference moves to the function on every return, errors included.

Takes ownership on success
  The reference moves only if the function succeeds. On error the caller
  still owns the buffer.

Putting a buffer in a FIFO or a list moves the reference to the queue.
The receiving side may process and free the buffer even before
:c:func:`k_fifo_put` returns, for instance when a higher-priority thread
is waiting on the FIFO. Move the reference with :c:func:`net_buf_take` in
the same expression:

.. code-block:: c

   k_fifo_put(&tx_queue, net_buf_take(&buf));

Do this even where ``buf`` goes out of scope right after the call: a
plain pointer argument looks the same as one that is only borrowed, while
:c:func:`net_buf_take` shows at the call site that the reference moves.

Passing ``net_buf_take(&buf)`` as an argument only works for calls that
always take ownership, such as :c:func:`k_fifo_put`,
:c:func:`net_buf_slist_put` and :c:func:`net_buf_frag_insert`. When a
function that takes ownership only on success is given a plain pointer,
the caller's pointer is unchanged either way, and only the return value
tells whether the buffer is still its own.

New functions that take ownership should therefore take a pointer to the
caller's buffer pointer, and move the reference out with
:c:func:`net_buf_take` when they take it. The caller's pointer is then
``NULL`` exactly when ownership has moved, and :c:func:`net_buf_drop` is
correct after the call whatever the outcome:

.. code-block:: c

   int foo_send(struct foo *foo, struct net_buf **buf)
   {
       if (!foo->ready) {
           return -EAGAIN;
       }

       k_fifo_put(&foo->tx_queue, net_buf_take(buf));

       return 0;
   }

   err = foo_send(foo, &buf);
   if (err != 0) {
       LOG_WRN("Not sent (err %d)", err);
   }

   /* Releases the buffer only if foo_send() did not take it */
   net_buf_drop(&buf);

Changing an existing function to this form breaks its callers, so it is
best done when the function is reworked anyway.

Debugging
*********

:kconfig:option:`CONFIG_NET_BUF_LOG` enables the logs of the network
buffer library. At the warning log level or higher, an allocation with
:c:macro:`K_FOREVER` that finds the pool empty then logs that the pool is
low on buffers, logs again every
:kconfig:option:`CONFIG_NET_BUF_WARN_ALLOC_INTERVAL` seconds for as long
as it is blocked, and logs how long it was blocked once it gets a buffer.
With the option set to 0 there are no periodic messages in between. This
points at the deadlock described in `Allocation Timeouts`_.

:kconfig:option:`CONFIG_NET_BUF_POOL_USAGE` tracks the use of each pool.
:c:func:`net_buf_get_available` returns the number of free buffers in a
pool and :c:func:`net_buf_get_max_used` the highest number of buffers in
use at the same time, which helps to size the pool. The log messages
then name the pool.

:kconfig:option:`CONFIG_NET_BUF_HARDENING` adds checks to the add,
remove, push and pull functions that do not depend on assertions. An
operation that would go outside the storage of the buffer, or that finds
the fields of the buffer inconsistent, is refused without touching the
data: functions that return a pointer return ``NULL``, those that return
a value return 0, and those that return nothing skip the write, so the
caller cannot always tell that the operation was refused. Without the
option, assertions check that an operation fits in the room the buffer
has, but not that its fields are consistent, and assertions are disabled
by default.
:c:func:`net_buf_is_valid` makes the same consistency check on demand,
and also checks that the buffer is still referenced.


API Reference
*************

.. doxygengroup:: net_buf
