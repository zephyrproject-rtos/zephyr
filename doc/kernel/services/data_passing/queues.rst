.. _queues:

Queues
######

A queue is a kernel object that implements a traditional queue, allowing threads
and ISRs to add and remove data items of any size. Items are stored as
single-linked nodes owned by the caller, so the kernel does not copy the data
being passed. The queue is similar to a FIFO and serves as the underlying
implementation for both :ref:`k_fifo <fifos_v2>` and :ref:`k_lifo <lifos_v2>`.
For more information on usage see :ref:`k_fifo <fifos_v2>`.

Safety
******

The safety relevance of the queue, the systematic capability it inherits, the
interfaces it shares with the hardware and the design decisions that follow
from them are documented separately in :ref:`queue_safety`.

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
* :kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE` sets the size of the system heap,
  which the allocating append and prepend variants use when they are called from
  ISR context, or from a thread that has been assigned the system pool with
  :c:func:`k_thread_system_pool_assign`. It does not on its own make those
  variants available to a thread: a thread with no resource pool assigned fails
  with ``-ENOMEM``.

API Reference
*************

.. doxygengroup:: queue_apis
