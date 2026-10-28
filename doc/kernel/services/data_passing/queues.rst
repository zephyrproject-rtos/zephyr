.. _queues:

Queues
######

A queue is a kernel object that implements a traditional queue, allowing threads
and ISRs to add and remove data items of any size. Items are stored as
single-linked nodes owned by the caller, so the kernel does not copy the data
being passed. The queue is similar to a FIFO and serves as the underlying
implementation for both :ref:`k_fifo <fifos_v2>` and :ref:`k_lifo <lifos_v2>`.
For more information on usage see :ref:`k_fifo <fifos_v2>`.

Safety Relevance
****************

The queue is a safety-related element of the kernel. Its relevance follows from
the role it plays rather than from an additional certification claim:

* It is the kernel primitive for passing data between contexts and is usable
  from both thread and ISR context.
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

Context constraints form the rest of the boundary: the non-blocking forms may be
called from ISR context, whereas the blocking forms are restricted to thread
context (see `Design Decisions`_).

Design Decisions
****************

**One lock per queue.**
Every mutation is performed under a single per-instance spinlock. Critical
sections contain no blocking operation and, in the non-allocating forms, no
allocation, so the latency of an operation is bounded and no lock ordering exists
between two queues.

**Blocking is delegated to the scheduler.**
A thread that finds the queue empty is pended on the queue's wait queue, and the
object lock is released as part of the context switch. Timeouts are handled by
the time subsystem rather than by the queue, which keeps timeout behaviour
uniform across kernel objects.

**Blocking from ISR context is refused fatally.**
A blocking call from ISR context would suspend an unrelated thread and, with
non-atomic context switching, corrupt scheduler state. Rather than degrade
silently, the kernel refuses such a call in every build. Fail-stop was chosen
because no safe recovery exists.

**Allocating forms delegate to the heap.**
The variants that create the node on behalf of the caller return a negative error
code when no memory is available instead of waiting for memory, so an allocation
shortfall is observable at the call site.

**User mode is validated on every call.**
With :kconfig:option:`CONFIG_USERSPACE`, each syscall validates the object and
the caller's permission before entering the implementation. A violation raises a
fatal kernel oops in the calling thread instead of returning an error, which
keeps the isolation boundary non-negotiable.

**No lock-free variant.**
A single lock over a linked list was preferred over a lock-free design: a
critical section that is short and easy to analyse is worth more in a safety
argument than added throughput.

Cancelling a Wait
*****************

A thread that is blocked waiting to retrieve an item from a queue can be released
without an item by calling :c:func:`k_queue_cancel_wait` from another thread or
an ISR. The first thread pending on the queue returns from its
:c:func:`k_queue_get` call with a ``NULL`` value, exactly as if its timeout had
expired. If the queue is instead being waited on through :c:func:`k_poll`, that
call returns ``-EINTR`` with the poll event in the cancelled state.

This mechanism is also available to FIFO objects through
:c:macro:`k_fifo_cancel_wait`.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_POLL` enables integration with :c:func:`k_poll`.
* :kconfig:option:`CONFIG_USERSPACE` enables object and permission validation for
  the queue system calls.
* :kconfig:option:`CONFIG_OBJ_CORE_QUEUE` registers queues with the object core;
  it does not change queue behaviour.
* :kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE` determines whether the allocating
  append and prepend variants can succeed.

API Reference
*************

.. doxygengroup:: queue_apis
