.. _queue_safety:

Queue
#####

Safety Relevance
****************

The queue is a safety-related element of the kernel. Its relevance follows from
the role it plays rather than from an additional certification claim:

* It is a kernel primitive used for passing data between contexts and is
  usable from both thread and ISR contexts.
* :ref:`k_fifo <fifos_v2>` and :ref:`k_lifo <lifos_v2>` are implemented on top
  of it, so a defect in the queue affects every user of those objects.

What the queue can contribute to a hazardous event is stated as requirements,
and is not repeated here: see `ZEP-SYRS-22`_. The certification scope and the
integrity level that apply to this page are defined in :ref:`safety_overview`.

Systematic Capability
*********************

The systematic capability claim for this component is inherited from the
project-level claim described in :ref:`safety_overview`. This page therefore
records only the two traces the component contributes to that claim.

**Requirements.** `ZEP-SYRS-22`_ states the queue requirements. The design
decisions below are the architecture elements they are allocated to; each one
records the behaviour that fixes a requirement rather than restating process.

**Integration tests.** :zephyr_file:`tests/kernel/queue` exercises the
behaviour described here, in the ``queue_api`` and ``queue_api_1cpu`` suites:
``test_queue_contexts.c`` covers the blocking and non-blocking forms and the
thread and ISR contexts, ``test_queue_fail.c`` the error paths,
``test_queue_loop.c`` the ordering guarantees, and ``test_queue_user.c`` the
userspace validation.

Software and Hardware Interactions
**********************************

The queue does not access hardware: it performs no MMIO, does not program a DMA
engine and does not interact with an interrupt controller directly. The
interfaces to the platform are limited to three:

* **Time.** A blocking get with a timeout is expressed in kernel time units and
  is serviced by the kernel time subsystem against the system clock. The queue
  itself holds no timer and no hardware deadline.
* **Interrupt masking.** Operations are serialized with an IRQ-masking spinlock,
  so the duration of an operation is a function of the number of items examined
  and not of any device latency.
* **Memory.** Items are caller-owned nodes and the queue stores pointers only. No
  buffer is copied and no cache maintenance is performed, so coherency of the
  payload remains the responsibility of the caller.

Context constraints form the rest of the boundary: ISR contexts are restricted
to calling the non-blocking forms, whereas thread contexts are permitted to call
both the non-blocking and the blocking forms (see `Design Decisions`_).

Design Decisions
****************

**One lock per queue.**
Every mutation is performed under a single per-instance spinlock. Critical
sections contain no blocking operation and, in the non-allocating forms, no
allocation, so the latency of an operation is bounded and no lock ordering exists
between two queues.

**Blocking is delegated to the scheduler.**
A thread that asks an empty queue for an item and gives a non-zero timeout is
pended on the queue's wait queue, and the object lock is released as part of the
context switch. Inserting is never blocking: the item is queued, and a thread
already waiting for one is woken instead. Timeouts are handled by the time
subsystem rather than by the queue, which keeps timeout behaviour uniform across
kernel objects.

**Blocking from ISR context is refused fatally.**
A blocking call from ISR context would suspend an unrelated thread and, with
non-atomic context switching, corrupt scheduler state. Rather than degrade
silently, the kernel refuses such a call in every build. Fail-stop was chosen
because no safe recovery exists.

**Allocating forms draw on the caller's resource pool.**
The variants that create the node on behalf of the caller obtain it from the
calling thread's resource pool (see :c:func:`k_thread_heap_assign` and
:c:func:`k_thread_system_pool_assign`) and fall back to the system heap in ISR
context. A thread normally has a pool: it inherits the pool of the thread that
created it, and the main thread starts on the system heap when
:kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE` is set, so the option does not by
itself make these variants unavailable. Only a thread left without a pool fails
with ``-ENOMEM``, whatever that option is set to. Nothing waits for memory to
become available, so a shortfall is observable at the call site. The allocation
is made with the queue lock held, so the heap lock nests inside the queue lock
and the cost of the allocator is added to the critical section; the latency
bound claimed for the non-allocating forms does not carry over to these
variants.

**User mode is validated on every call.**
With :kconfig:option:`CONFIG_USERSPACE`, each syscall validates the object and
the caller's permission before entering the implementation. A violation raises a
fatal kernel oops in the calling thread instead of returning an error, which
keeps the isolation boundary non-negotiable.

**No lock-free variant.**
A single lock over a linked list was preferred over a lock-free design: a
critical section that is short and easy to analyse is worth more in a safety
argument than added throughput.

.. _ZEP-SYRS-22: https://docs.zephyrproject.org/latest/build/requirements/zephyr_system_requirements.html#zep-syrs-22-queues-data-passing
