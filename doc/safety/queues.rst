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
* The failure modes that matter are loss, duplication or reordering of an item,
  a spurious wakeup, and a missed wakeup. Each of them can propagate into
  application behaviour that contributes to a hazardous event.

The certification scope and the integrity level that apply to this document are
defined in :ref:`safety_overview`.

Systematic Capability
*********************

The systematic capability claim for this component is inherited from the
project-level claim described in :ref:`safety_overview`. This section records how
the component supports that claim rather than establishing a claim of its own:

* Development follows the Zephyr coding guidelines and is subject to peer review.
* Static analysis and license scanning run on every change.
* The behaviour described in this document is traced to requirements in the
  requirements management repository, and this page is the architecture entry for
  that trace.
* Regression tests are maintained in :zephyr_file:`tests/kernel/queue`.

No component-specific restriction on the project claim is currently identified.
Any limitation found during the safety assessment is recorded in this section.

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
The variants that create the node on behalf of the caller obtain it with
``z_thread_malloc()``, which uses the calling thread's resource pool (see
:c:func:`k_thread_heap_assign` and :c:func:`k_thread_system_pool_assign`) and the
system heap only when called from ISR context. A thread that has no resource pool
assigned fails with ``-ENOMEM`` regardless of
:kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE`. Nothing waits for memory to become
available, so a shortfall is observable at the call site. The allocation is made
with the queue lock held, so the heap lock nests inside the queue lock and the
cost of the allocator is added to the critical section; the latency bound claimed
for the non-allocating forms does not carry over to these variants.

**User mode is validated on every call.**
With :kconfig:option:`CONFIG_USERSPACE`, each syscall validates the object and
the caller's permission before entering the implementation. A violation raises a
fatal kernel oops in the calling thread instead of returning an error, which
keeps the isolation boundary non-negotiable.

**No lock-free variant.**
A single lock over a linked list was preferred over a lock-free design: a
critical section that is short and easy to analyse is worth more in a safety
argument than added throughput.
