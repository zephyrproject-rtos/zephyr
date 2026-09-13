.. _workq_api:

Work Queue (workq)
##################

.. contents::
  :local:
  :depth: 2

Overview
********

The ``workq`` module provides a queue-centric work queue that defers tasks from interrupt context
or high-priority threads to one or more dedicated worker threads. It complements, rather than
replaces, the :ref:`k_work <workqueues_v2>` API and can be used side by side with it.

Where the :ref:`k_work <workqueues_v2>` API stores lifecycle state inside each work item, ``workq``
infers the state of a work item from its relationship with the queue it was submitted to. This shift
addresses several structural limitations of the item-centric model:

* **Lifecycle management**:  Because state is owned by the queue, a work item may be safely freed
  (or re-submitted) from within its own callback. This enables *fire-and-forget*, dynamically
  allocated work items.
* **Parallel execution**: The queue is decoupled from the worker thread, so multiple worker
  threads may process a single queue in parallel. The queue stays FIFO on dequeue, but completion
  order may vary with callback duration, thread priority, and scheduling.
* **Unified work items**: A single :c:struct:`work_item` type is used for both immediate and
  delayed work, keeping the item struct small.
* **Deterministic teardown**: The queue holds references to every submitted item, including delayed
  ones, which makes it possible to stop, freeze, and reclaim a queue deterministically.

.. note::

   The ``workq`` API is experimental. The naming (the ``workq_*`` prefix) and placement are still
   under discussion and may change in future releases.

.. note::

   Unlike the :ref:`k_work <workqueues_v2>` API, ``workq`` intentionally omits ``sync()`` support.
   Because a work item may free or re-queue itself in its own callback, a waiting thread cannot
   reliably distinguish the original task from a re-queued or newly allocated item that reuses the
   same address. Synchronization, when required, must be done explicitly at the application level
   (see :ref:`workq_synchronization`).

Concepts and Structures
***********************

The module is built from four cooperating structures:

* :c:struct:`workq` - The queue itself. It owns three intrusive lists - ``pending`` (ready to run),
  ``delayed`` (waiting for their execution time), and ``active`` (currently running) — together with
  the single timeout used to schedule delayed work.
* :c:struct:`work_item` - A unit of work: a callback plus the bookkeeping node. The same type is
  used for immediate and delayed work.
* :c:struct:`workq_thread` - A worker thread bound to a queue. Several worker threads may be bound to
  the same queue.
* :c:struct:`workq_thread_config` - Static configuration (name and priority) for a worker thread.

Because a work item is intrusive, it is normally embedded in an application-defined struct and
recovered with :c:macro:`CONTAINER_OF` inside the callback:

.. code-block:: c

   struct container {
           struct work_item item;
           size_t number;
   };

   static void work_fn(struct work_item *item)
   {
           struct container *c = CONTAINER_OF(item, struct container, item);
           /* ... use c ... */
           k_free(c); /* safe: the item may be freed from its own callback */
   }

State Machines
**************

Work item
=========

A work item moves between *initialized*, *pending* (queued or delayed), and *running*. It cannot be
cancelled while running, but during the callback the item may be safely modified, freed, or
re-submitted.

.. mermaid::

   stateDiagram-v2
   [*] --> INITIALIZED : work_init()

   INITIALIZED --> PENDING : workq_submit() / workq_delayed_submit()
   INITIALIZED --> PENDING : workq_reschedule()
   PENDING --> PENDING : workq_reschedule()

   PENDING --> INITIALIZED : workq_cancel()
   PENDING --> RUNNING : workq_run()

   RUNNING --> PENDING : [re-submitted in callback]
   RUNNING --> INITIALIZED : [not re-submitted]

   note right of RUNNING
     Cannot be cancelled.
     The work item can be safely modified, freed, or re-submitted.
   end note

Queue
=====

A queue is either ``OPEN`` (accepting new work) or ``CLOSED`` (rejecting it), and orthogonally either
running or ``FROZEN``. These two axes are independent: :c:func:`workq_open` / :c:func:`workq_close`
control only whether submissions are accepted, and :c:func:`workq_freeze` / :c:func:`workq_thaw`
control only whether delayed items are scheduled. Freezing keeps processing pending items but stops
scheduling delayed items until the queue is thawed, without affecting whether new work is accepted.

.. mermaid::

   stateDiagram-v2
   [*] --> OPEN : workq_init()

   state "OPEN" as OPEN {
     [*] --> OPEN_RUNNING
     OPEN_RUNNING
     OPEN_FROZEN
     OPEN_RUNNING --> OPEN_FROZEN : workq_freeze()
     OPEN_FROZEN --> OPEN_RUNNING : workq_thaw()

     note right of OPEN_RUNNING
         Accepting new work.
         Processing pending and scheduling delayed items.
     end note

     note right of OPEN_FROZEN
         Accepting new work.
         Pending items run, but delayed items are
         not scheduled until thawed.
     end note
   }

   state "CLOSED" as CLOSED {
     CLOSED_RUNNING
     CLOSED_FROZEN
     CLOSED_RUNNING --> CLOSED_FROZEN : workq_freeze()
     CLOSED_FROZEN --> CLOSED_RUNNING : workq_thaw()

     note right of CLOSED_RUNNING
         Rejecting new work
         (workq_submit -EAGAIN, workq_delayed_submit -EAGAIN).
         Processing pending and scheduling delayed items.
     end note

     note right of CLOSED_FROZEN
         Rejecting new work
         (workq_submit -EAGAIN, workq_delayed_submit -EAGAIN).
         Pending items run, but delayed items are
         not scheduled until thawed.
     end note
   }

   OPEN_RUNNING --> CLOSED_RUNNING : workq_close()
   CLOSED_RUNNING --> OPEN_RUNNING : workq_open()
   OPEN_FROZEN --> CLOSED_FROZEN : workq_close()
   CLOSED_FROZEN --> OPEN_FROZEN : workq_open()

Worker thread
=============

A worker thread is initialized and then started; while running it repeatedly pulls and executes work
from its queue until stopped.

.. mermaid::

   stateDiagram-v2
   [*] --> UNALLOCATED

    UNALLOCATED --> INITIALIZED : workq_thread_init()
    UNALLOCATED --> RUNNING : WORKQ_THREAD_DEFINE() (static)

    INITIALIZED --> RUNNING : workq_thread_start()

    RUNNING --> INITIALIZED : workq_thread_stop()
    RUNNING --> INITIALIZED : workq_run() error (self-exit)


    note right of INITIALIZED
         The work queue thread is initialized but not running.
    end note
    note right of RUNNING
         The work queue thread is running and can process work items.
    end note

Instantiation and Usage
***********************

A queue can be defined statically with :c:macro:`WORKQ_DEFINE` or initialized at runtime with
:c:func:`workq_init`. Worker threads are defined with :c:macro:`WORKQ_THREAD_DEFINE`, or created at
runtime with :c:func:`workq_thread_init` followed by :c:func:`workq_thread_start`. A thread defined
with :c:macro:`WORKQ_THREAD_DEFINE` starts in the running state and does not require an explicit
:c:func:`workq_thread_start`.

For finer control over thread configuration, :c:macro:`WORKQ_THREAD_CONFIG` defines a
:c:struct:`workq_thread_config` (name and priority) statically, :c:macro:`WORKQ_THREAD_CONFIG_INITIALIZER`
provides an initializer for one embedded in another structure, and :c:macro:`WORKQ_THREAD_INITIALIZE`
initializes a :c:struct:`workq_thread` from an existing config, stack, and queue.

.. code-block:: c

   #include <zephyr/workq.h>

   #define PRIORITY 0

   WORKQ_DEFINE(my_workq);
   WORKQ_THREAD_DEFINE(worker1, my_workq, 1024, PRIORITY);
   WORKQ_THREAD_DEFINE(worker2, my_workq, 1024, PRIORITY);

A work item is initialized with :c:func:`work_init` and submitted with :c:func:`workq_submit`:

.. code-block:: c

   struct container *c = k_malloc(sizeof(*c));

   work_init(&c->item, work_fn);
   c->number = 0;
   workq_submit(&my_workq, &c->item);

Submitting and Managing Work
============================

* :c:func:`workq_submit` - Enqueue an item for immediate processing. Returns ``-EAGAIN`` if the
  queue is closed and ``-EALREADY`` if the item is already pending.
* :c:func:`workq_delayed_submit` - Enqueue an item to run after a delay. Returns ``-EAGAIN`` if the
  queue is closed and ``-EALREADY`` if the item is already pending.
* :c:func:`workq_reschedule` - Cancel a pending or delayed item if present and (re-)submit it with a
  new delay. Returns ``-EAGAIN`` if the queue is closed. If the item is currently running it is not
  "pending", so it is not cancelled and is simply submitted again as delayed work.
* :c:func:`workq_cancel` - Remove a pending or delayed item. Returns ``-EBUSY`` if the item is
  running and ``-ENOENT`` if it is not queued.
* :c:func:`workq_run` - Process a single work item, blocking up to ``timeout`` for one to become
  available. Returns ``-EAGAIN`` on timeout. This is the primitive a worker thread loops on; it can
  also be called directly to run a queue on an existing thread (for example ``main()``).
* :c:func:`workq_drain` - Block up to ``timeout`` until the queue is idle (no pending, delayed, or
  active items). Returns ``-EAGAIN`` on timeout.

The return codes are summarized below:

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Function
     - Return values
   * - :c:func:`workq_submit`
     - ``0`` on success, ``-EAGAIN`` if the queue is closed, ``-EALREADY`` if already pending
   * - :c:func:`workq_delayed_submit`
     - ``0`` on success, ``-EAGAIN`` if the queue is closed, ``-EALREADY`` if already pending
   * - :c:func:`workq_reschedule`
     - ``0`` on success, ``-EAGAIN`` if the queue is closed (a running item is re-submitted as delayed work)
   * - :c:func:`workq_cancel`
     - ``0`` on success, ``-EBUSY`` if the item is running, ``-ENOENT`` if it is not queued
   * - :c:func:`workq_run`
     - ``0`` if a work item was executed, ``-EAGAIN`` on timeout
   * - :c:func:`workq_drain`
     - ``0`` if the queue drained, ``-EAGAIN`` on timeout

Multiple Worker Threads
=======================

Binding several :c:struct:`workq_thread` instances to one queue lets I/O-bound work be processed in
parallel. Items are dequeued in FIFO order, but because callbacks may run for different durations,
their completion order is not guaranteed.

Delayed Work
============

Delayed work items are managed by the queue itself, which keeps a single timeout for the earliest
delayed item rather than a timer per item. When an item's execution time is reached it is moved from
the ``delayed`` list to the ``pending`` list. Because the queue tracks every delayed item, they are
also visible to teardown.

Open/Close and Freeze/Thaw
==========================

* :c:func:`workq_open` / :c:func:`workq_close` control whether the queue accepts new submissions.
  A closed queue still processes work already queued.
* :c:func:`workq_freeze` sets the frozen state and aborts the delayed-work timeout, so delayed items
  are not promoted to pending. :c:func:`workq_thaw` clears the frozen state and reschedules delayed
  work. Freeze/thaw are orthogonal to open/close: they do not change whether the queue accepts new
  submissions, and a delayed item submitted while frozen is simply queued and scheduled on thaw.

Deterministic Teardown
======================

Because the queue references every submitted item, a subsystem can be torn down deterministically:

#. Stop the worker thread(s) with :c:func:`workq_thread_stop`. This function takes a ``timeout`` and
   returns the result of joining the thread (for example ``-EAGAIN`` if the join times out).
#. :c:func:`workq_close` the queue to prevent new submissions, and :c:func:`workq_freeze` it to stop
   delayed scheduling.
#. Reclaim any resources associated with the work items and the queue.

.. _workq_synchronization:

Synchronization
===============

``workq`` does not provide a ``sync()`` primitive. When a thread must block until a specific work
item has finished, it should carry its own synchronization object, such as a semaphore, and the
callback should signal it:

.. code-block:: c

   struct sync {
           struct work_item item;
           struct k_sem sem;
   };

   static void sync_fn(struct work_item *item)
   {
           struct sync *s = CONTAINER_OF(item, struct sync, item);

           /* ... do work ... */
           k_sem_give(&s->sem);
   }

   void submit_and_wait(void)
   {
           struct sync *s = k_malloc(sizeof(*s));

           work_init(&s->item, sync_fn);
           k_sem_init(&s->sem, 0, 1);
           workq_submit(&my_workq, &s->item);
           k_sem_take(&s->sem, K_FOREVER);
           k_free(s); /* the item outlives the callback; free it here */
   }

A complete, runnable example is available in the :zephyr:code-sample:`workq` sample.

API Reference
*************

.. doxygengroup:: workq_apis
