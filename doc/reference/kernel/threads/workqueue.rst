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

Any number of workqueues can be defined. Each workqueue is referenced by its
memory address.

A workqueue has the following key properties:

* A **queue** of work items that have been added, but not yet processed.

* A **thread** that processes the work items in the queue. The priority of the
  thread is configurable, allowing it to be either cooperative or preemptive
  as required.

A workqueue must be initialized before it can be used. This sets its queue
to empty and spawns the workqueue's thread.

Work Item Lifecycle
********************

Any number of **work items** can be defined. Each work item is referenced
by its memory address.

A work item has the following key properties:

* A **handler function**, which is the function executed by the workqueue's
  thread when the work item is processed. This function accepts a single
  argument, which is the address of the work item itself.

* A **pending flag**, which is used by the kernel to signify that the
  work item is currently a member of a workqueue's queue.

* A **queue link**, which is used by the kernel to link a pending work
  item to the next pending work item in a workqueue's queue.

A work item must be initialized before it can be used. This records the work
item's handler function and marks it as not pending.

A work item may be **submitted** to a workqueue by an ISR or a thread.
Submitting a work item appends the work item to the workqueue's queue.
Once the workqueue's thread has processed all of the preceding work items
in its queue the thread will remove a pending work item from its queue and
invoke the work item's handler function. Depending on the scheduling priority
of the workqueue's thread, and the work required by other items in the queue,
a pending work item may be processed quickly or it may remain in the queue
for an extended period of time.

A handler function can utilize any kernel API available to threads. However,
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
    while it is pending. Furthermore, any additional information the work item's
    handler function needs to perform its work must not be altered until
    the handler function has finished executing.

Delayed Work
************

An ISR or a thread may need to schedule a work item that is to be processed
only after a specified period of time, rather than immediately. This can be
done by submitting a **delayed work item** to a workqueue, rather than a
standard work item.

A delayed work item is a standard work item that has the following added
properties:

* A **delay** specifying the time interval to wait before the work item
  is actually submitted to a workqueue's queue.

* A **workqueue indicator** that identifies the workqueue the work item
  is to be submitted to.

A delayed work item is initialized and submitted to a workqueue in a similar
manner to a standard work item, although different kernel APIs are used.
When the submit request is made the kernel initiates a timeout mechanism
that is triggered after the specified delay has elapsed. Once the timeout
occurs the kernel submits the delayed work item to the specified workqueue,
where it remains pending until it is processed in the standard manner.

An ISR or a thread may **cancel** a delayed work item it has submitted,
providing the work item's timeout is still counting down. The work item's
timeout is aborted and the specified work is not performed.

Attempting to cancel a delayed work item once its timeout has expired has
no effect on the work item; the work item remains pending in the workqueue's
queue, unless the work item has already been removed and processed by the
workqueue's thread. Consequently, once a work item's timeout has expired
the work item is always processed by the workqueue and cannot be canceled.

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

Defining a Workqueue
====================

A workqueue is defined using a variable of type :c:struct:`k_work_q`.
The workqueue is initialized by defining the stack area used by its thread
and then calling :c:func:`k_work_q_start`. The stack area must be defined
using :c:macro:`K_THREAD_STACK_DEFINE` to ensure it is properly set up in
memory.

The following code defines and initializes a workqueue.

.. code-block:: c

    #define MY_STACK_SIZE 512
    #define MY_PRIORITY 5

    K_THREAD_STACK_DEFINE(my_stack_area, MY_STACK_SIZE);

    struct k_work_q my_work_q;

    k_work_q_start(&my_work_q, my_stack_area,
                   K_THREAD_STACK_SIZEOF(my_stack_area), MY_PRIORITY);

Submitting a Work Item
======================

A work item is defined using a variable of type :c:struct:`k_work`.
It must then be initialized by calling :c:func:`k_work_init`.

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

Submitting a Delayed Work Item
==============================

A delayed work item is defined using a variable of type
:c:struct:`k_delayed_work`. It must then be initialized by calling
:c:func:`k_delayed_work_init`.

An initialized delayed work item can be submitted to the system workqueue by
calling :c:func:`k_delayed_work_submit`, or to a specified workqueue by
calling :c:func:`k_delayed_work_submit_to_queue`. A delayed work item
that has been submitted but not yet consumed by its workqueue can be canceled
by calling :c:func:`k_delayed_work_cancel`.

Suggested Uses
**************

Use the system workqueue to defer complex interrupt-related processing
from an ISR to a cooperative thread. This allows the interrupt-related
processing to be done promptly without compromising the system's ability
to respond to subsequent interrupts, and does not require the application
to define an additional thread to do the processing.



Configuration Options
**********************

Related configuration options:

* :option:`CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE`
* :option:`CONFIG_SYSTEM_WORKQUEUE_PRIORITY`
