.. _workqueues_v2:

Workqueue Threads
#################

.. contents::
    :local:
    :depth: 1

A :dfn:`workqueue` is a kernel object that uses a dedicated thread to process
work items in a first in, first out manner. Each work item is processed by
calling the function specified by the work item. A workqueue is typically
used by an ISR or a high-priority thread to offload non-urgent processing
to a lower-priority thread so it does not impact time-sensitive processing.

Any number of workqueues can be defined (limited only by available RAM). Each
workqueue is referenced by its memory address.

A workqueue has the following key properties:

* A **queue** of work items that have been added, but not yet processed.

* A **thread** that processes the work items in the queue. The priority of the
  thread is configurable, allowing it to be either cooperative or preemptive
  as required.

Regardless of workqueue thread priority the workqueue thread will yield
between each submitted work item, to prevent a cooperative workqueue from
starving other threads.

A workqueue must be initialized before it can be used. This sets its queue to
empty and spawns the workqueue's thread.  The thread runs forever, but sleeps
when no work items are available.

.. note::
   The behavior described here is changed from the Zephyr workqueue
   implementation used prior to release 2.6.  Among the changes are:

   * Precise tracking of the status of cancelled work items, so that the
     caller need not be concerned that an item may be processing when the
     cancellation returns.  Checking of return values on cancellation is still
     required.
   * Direct submission of delayable work items to the queue with
     :c:macro:`K_NO_WAIT` rather than always going through the timeout API,
     which could introduce delays.
   * The ability to wait until a work item has completed or a queue has been
     drained.
   * Finer control of behavior when scheduling a delayable work item,
     specifically allowing a previous deadline to remain unchanged when a work
     item is scheduled again.
   * Safe handling of work item resubmission when the item is being processed
     on another workqueue.

   Using the return values of :c:func:`k_work_busy_get()` or
   :c:func:`k_work_is_pending()`, or measurements of remaining time until
   delayable work is scheduled, should be avoided to prevent race conditions
   of the type observed with the previous implementation.

Work Item Lifecycle
********************

Any number of **work items** can be defined. Each work item is referenced
by its memory address.

A work item is assigned a **handler function**, which is the function
executed by the workqueue's thread when the work item is processed. This
function accepts a single argument, which is the address of the work item
itself.  The work item also maintains information about its status.

A work item must be initialized before it can be used. This records the work
item's handler function and marks it as not pending.

A work item may be **submitted** (:c:macro:`K_WORK_QUEUED`) to a workqueue by
an ISR or a thread.  Submitting a work item appends the work item to the
workqueue's queue.  Once the workqueue's thread has processed all of the
preceding work items in its queue the thread will remove a pending work item
from its queue and invoke the work item's handler function. Depending on the
scheduling priority of the workqueue's thread, and the work required by other
items in the queue, a pending work item may be processed quickly or it may
remain in the queue for an extended period of time.

A delayable work item may be **scheduled** (:c:macro:`K_WORK_DELAYED`) to a
workqueue; see `Delayable Work`_.

A work item will be **running** (:c:macro:`K_WORK_RUNNING`) when it is running
on a work queue, and may also be **canceling** (:c:macro:`K_WORK_CANCELING`)
if it started running before a thread has requested that it be cancelled.

A work item can be in multiple states; for example it can be:

* running on a queue;
* marked canceling (because a thread used :c:func:`k_work_cancel_sync()` to
  wait until the work item completed);
* queued to run again on the same queue;
* scheduled to be submitted to a (possibly different) queue

*all simultaneously*.  A work item that is in any of these states is **pending**
(:c:func:`k_work_is_pending()`) or **busy** (:c:func:`k_work_busy_get()`).

A handler function can use any kernel API available to threads. However,
operations that are potentially blocking (e.g. taking a semaphore) must be
used with care, since the workqueue cannot process subsequent work items in
its queue until the handler function finishes executing.

The single argument that is passed to a handler function can be ignored if
it is not required. If the handler function requires additional information
about the work it is to perform, the work item can be embedded in a larger
data structure. The handler function can then use the argument value to compute
the address of the enclosing data structure, and thereby obtain access to the
additional information it needs.

A work item is typically initialized once and then submitted to a specific
workqueue whenever work needs to be performed. If an ISR or a thread attempts
to submit a work item that is already pending, the work item is not affected;
the work item remains in its current place in the workqueue's queue, and
the work is only performed once.

A handler function is permitted to re-submit its work item argument
to the workqueue, since the work item is no longer pending at that time.
This allows the handler to execute work in stages, without unduly delaying
the processing of other work items in the workqueue's queue.

.. important::
    A pending work item *must not* be altered until the item has been processed
    by the workqueue thread. This means a work item must not be re-initialized
    while it is busy. Furthermore, any additional information the work item's
    handler function needs to perform its work must not be altered until
    the handler function has finished executing.

.. _k_delayable_work:

Delayable Work
**************

An ISR or a thread may need to schedule a work item that is to be processed
only after a specified period of time, rather than immediately. This can be
done by **scheduling** a **delayable work item** to be submitted to a
workqueue at a future time.

A delayable work item contains a standard work item but adds fields that
record when and where the item should be submitted.

A delayable work item is initialized and scheduled to a workqueue in a similar
manner to a standard work item, although different kernel APIs are used.  When
the schedule request is made the kernel initiates a timeout mechanism that is
triggered after the specified delay has elapsed. Once the timeout occurs the
kernel submits the work item to the specified workqueue, where it remains
pending until it is processed in the standard manner.

Triggered Work
**************

The :c:func:`k_work_poll_submit` interface schedules a triggered work
item in response to a **poll event** (see :ref:`polling_v2`), that will
call a user-defined function when a monitored resource becomes available
or poll signal is raised, or a timeout occurs.
In contrast to :c:func:`k_poll`, the triggered work does not require
a dedicated thread waiting or actively polling for a poll event.

A triggered work item is a standard work item that has the following
added properties:

* A pointer to an array of poll events that will trigger work item
  submissions to the workqueue

* A size of the array containing poll events.

A triggered work item is initialized and submitted to a workqueue in a similar
manner to a standard work item, although dedicated kernel APIs are used.
When a submit request is made, the kernel begins observing kernel objects
specified by the poll events. Once at least one of the observed kernel
object's changes state, the work item is submitted to the specified workqueue,
where it remains pending until it is processed in the standard manner.

.. important::
    The triggered work item as well as the referenced array of poll events
    have to be valid and cannot be modified for a complete triggered work
    item lifecycle, from submission to work item execution or cancellation.

An ISR or a thread may **cancel** a triggered work item it has submitted
as long as it is still waiting for a poll event. In such case, the kernel
stops waiting for attached poll events and the specified work is not executed.
Otherwise the cancellation cannot be performed.

System Workqueue
*****************

The kernel defines a workqueue known as the *system workqueue*, which is
available to any application or kernel code that requires workqueue support.
The system workqueue is optional, and only exists if the application makes
use of it.

.. important::
    Additional workqueues should only be defined when it is not possible
    to submit new work items to the system workqueue, since each new workqueue
    incurs a significant cost in memory footprint. A new workqueue can be
    justified if it is not possible for its work items to co-exist with
    existing system workqueue work items without an unacceptable impact;
    for example, if the new work items perform blocking operations that
    would delay other system workqueue processing to an unacceptable degree.

Implementation
**************

Defining and Controlling a Workqueue
====================================

A workqueue is defined using a variable of type :c:struct:`k_work_q`.
The workqueue is initialized by defining the stack area used by its thread
and then calling :c:func:`k_work_queue_start`. The stack area must be defined
using :c:macro:`K_THREAD_STACK_DEFINE` to ensure it is properly set up in
memory.

The following code defines and initializes a workqueue.

.. code-block:: c

    #define MY_STACK_SIZE 512
    #define MY_PRIORITY 5

    K_THREAD_STACK_DEFINE(my_stack_area, MY_STACK_SIZE);

    struct k_work_q my_work_q;

    k_work_queue_start(&my_work_q, my_stack_area,
                       K_THREAD_STACK_SIZEOF(my_stack_area), MY_PRIORITY,
		       NULL);

In addition the queue identity and certain behavior related to thread
rescheduling can be controlled by the optional final parameter; see
:c:struct:`k_work_queue_start()` for details.

The following API can be used to interact with a workqueue:

* :c:func:`k_work_queue_drain()` can be used to block the caller until no more
  items are pending for the queue.  Work items resubmitted from the workqueue
  thread are accepted while a queue is draining, but work items from any other
  thread or ISR are rejected.  The restriction on submitting more work can be
  extended past the completion of the drain operation in order to allow the
  blocking thread to perform additional work while the queue is "plugged".
* :c:func:`k_work_queue_unplug()` removes any previous block on submission to
  the queue due to a previous drain operation.

Submitting a Work Item
======================

A work item is defined using a variable of type :c:struct:`k_work`.  It must
be initialized by calling :c:func:`k_work_init`.

An initialized work item can be submitted to the system workqueue by
calling :c:func:`k_work_submit`, or to a specified workqueue by
calling :c:func:`k_work_submit_to_queue`.

The following code demonstrates how an ISR can offload the printing
of error messages to the system workqueue. Note that if the ISR attempts
to resubmit the work item while it is still pending, the work item is left
unchanged and the associated error message will not be printed.

.. code-block:: c

    struct device_info {
        struct k_work work;
        char name[16]
    } my_device;

    void my_isr(void *arg)
    {
        ...
        if (error detected) {
            k_work_submit(&my_device.work);
	}
	...
    }

    void print_error(struct k_work *item)
    {
        struct device_info *the_device =
            CONTAINER_OF(item, struct device_info, work);
        printk("Got error on device %s\n", the_device->name);
    }

    /* initialize name info for a device */
    strcpy(my_device.name, "FOO_dev");

    /* initialize work item for printing device's error messages */
    k_work_init(&my_device.work, print_error);

    /* install my_isr() as interrupt handler for the device (not shown) */
    ...


The following API can be used to check the status of or synchronize with the
work item:

* :c:func:`k_work_busy_get()` returns a snapshot of flags indicating work item
  state.  A zero value indicates the work is not scheduled, submitted, being
  executed, or otherwise still being referenced by the workqueue
  infrastructure.
* :c:func:`k_work_is_pending()` is a helper that indicates ``true`` if and only
  if the work is scheduled, submitted, or being executed.
* :c:func:`k_work_flush()` may be invoked from threads to block until the work
  item has completed.  It returns immediately if the work is not pending.
* :c:func:`k_work_cancel()` attempts to prevent the work item from being
  executed.  This may or may not be successful. This is safe to invoke
  from ISRs.
* :c:func:`k_work_cancel_sync()` may be invoked from threads to block until
  the work completes; it will return immediately if the cancellation was
  successful or not necessary (the work wasn't submitted or running).  This
  can be used after :c:func:`k_work_cancel()` is invoked (from an ISR)` to
  confirm completion of an ISR-initiated cancellation.

Submitting a Delayable Work Item
================================

A delayable work item is defined using a variable of type
:c:struct:`k_work_delayable`. It must be initialized by calling
:c:func:`k_work_init_delayable`.

There are two APIs that submit delayable work:

* :c:func:`k_work_schedule()` (or :c:func:`k_work_schedule_for_queue()`)
  schedules work to be executed at a specific time or after a delay.  Further
  attempts to schedule the same item with this API before the delay completes
  will not change the time at which the item will be submitted to its queue.

* :c:func:`k_work_reschedule()` (or :c:func:`k_work_reschedule_for_queue()`)
  unconditionally sets the deadline for the work, replacing any previous
  incomplete delay and changing the destination queue if necessary.

If the work item is not scheduled both APIs behave the same.  If
:c:macro:`K_NO_WAIT` is specified as the delay the behavior is as if the item
was immediately submitted directly to the target queue, without waiting for a
minimal timeout (unless :c:func:`k_work_schedule()` is used and a previous
delay has not completed).

Both also have variants that allow
control of the queue used for submission.

The helper function :c:func:`k_work_delayable_from_work()` can be used to get
a pointer to the containing :c:struct:`k_work_delayable` from a pointer to
:c:struct:`k_work` that is passed to a work handler function.

The following additional API can be used to check the status of or synchronize
with the work item:

* :c:func:`k_work_delayable_busy_get()` is the analog to :c:func:`k_work_busy_get()`
  for delayable work.
* :c:func:`k_work_delayable_is_pending()` is the analog to
  :c:func:`k_work_is_pending()` for delayable work.
* :c:func:`k_work_flush_delayable()` is the analog to :c:func:`k_work_flush()`
  for delayable work.
* :c:func:`k_work_cancel_delayable()` is the analog to
  :c:func:`k_work_cancel()` for delayable work; similarly with
  :c:func:`k_work_cancel_delayable_sync()`.

Synchronizing with Work Items
=============================

While the state of both regular and delayable work items can be determined
from any context using :c:func:`k_work_busy_get()` and
:c:func:`k_work_delayable_busy_get()` some use cases require synchronizing
with work items after they've been submitted.  :c:func:`k_work_flush()`,
:c:func:`k_work_cancel_sync()`, and :c:func:`k_work_cancel_delayable_sync()`
can be invoked from thread context to wait until the requested state has been
reached.

These APIs must be provided with a :c:struct:`k_work_sync` object that has no
application-inspectable components but is needed to provide the
synchronization objects.  These objects should not be allocated on a stack if
the code is expected to work on architectures with
:option:`CONFIG_KERNEL_COHERENCE`.


Suggested Uses
**************

Use the system workqueue to defer complex interrupt-related processing from an
ISR to a shared thread. This allows the interrupt-related processing to be
done promptly without compromising the system's ability to respond to
subsequent interrupts, and does not require the application to define and
manage an additional thread to do the processing.

Configuration Options
**********************

Related configuration options:

* :option:`CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE`
* :option:`CONFIG_SYSTEM_WORKQUEUE_PRIORITY`
* :option:`CONFIG_SYSTEM_WORKQUEUE_NO_YIELD`
