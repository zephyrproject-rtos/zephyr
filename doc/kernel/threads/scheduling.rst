.. _scheduling_v2:

Scheduling
##########

The kernel's priority-based scheduler allows an application's threads
to share the CPU.

.. contents::
    :local:
    :depth: 2

Concepts
********

The scheduler determines which thread is allowed to execute
at any point in time; this thread is known as the **current thread**.

Whenever the scheduler changes the identity of the current thread,
or when execution of the current thread is supplanted by an ISR,
the kernel first saves the current thread's CPU register values.
These register values get restored when the thread later resumes execution.

Thread States
=============

A thread that has no factors that prevent its execution is deemed
to be **ready**, and is eligible to be selected as the current thread.

A thread that has one or more factors that prevent its execution
is deemed to be **unready**, and cannot be selected as the current thread.

The following factors make a thread unready:

* The thread has not been started.
* The thread is waiting on for a kernel object to complete an operation.
  (For example, the thread is taking a semaphore that is unavailable.)
* The thread is waiting for a timeout to occur.
* The thread has been suspended.
* The thread has terminated or aborted.

Thread Priorities
=================

A thread's priority is an integer value, and can be either negative or
non-negative.
Numerically lower priorities takes precedence over numerically higher values.
For example, the scheduler gives thread A of priority 4 *higher* priority
over thread B of priority 7; likewise thread C of priority -2 has higher
priority than both thread A and thread B.

The scheduler distinguishes between two classes of threads,
based on each thread's priority.

* A :dfn:`cooperative thread` has a negative priority value.
  Once it becomes the current thread, a cooperative thread remains
  the current thread until it performs an action that makes it unready.

* A :dfn:`preemptible thread` has a non-negative priority value.
  Once it becomes the current thread, a preemptible thread may be supplanted
  at any time if a cooperative thread, or a preemptible thread of higher
  or equal priority, becomes ready.

A thread's initial priority value can be altered up or down after the thread
has been started. Thus it possible for a preemptible thread to become
a cooperative thread, and vice versa, by changing its priority.

The kernel supports a virtually unlimited number of thread priority levels.
The configuration options :option:`CONFIG_NUM_COOP_PRIORITIES` and
:option:`CONFIG_NUM_PREEMPT_PRIORITIES` specify the number of priority
levels for each class of thread, resulting the following usable priority
ranges:

* cooperative threads: (-:option:`CONFIG_NUM_COOP_PRIORITIES`) to -1
* preemptive threads: 0 to (:option:`CONFIG_NUM_PREEMPT_PRIORITIES` - 1)

For example, configuring 5 cooperative priorities and 10 preemptive priorities
results in the ranges -5 to -1 and 0 to 9, respectively.

Scheduling Algorithm
====================

The kernel's scheduler selects the highest priority ready thread
to be the current thread. When multiple ready threads of the same priority
exist, the scheduler chooses the one that has been waiting longest.

.. note::
    Execution of ISRs takes precedence over thread execution,
    so the execution of the current thread may be supplanted by an ISR
    at any time unless interrupts have been masked. This applies to both
    cooperative threads and preemptive threads.

Cooperative Time Slicing
========================

Once a cooperative thread becomes the current thread, it remains
the current thread until it performs an action that makes it unready.
Consequently, if a cooperative thread performs lengthy computations,
it may cause an unacceptable delay in the scheduling of other threads,
including those of higher priority and equal priority.

To overcome such problems, a cooperative thread can voluntarily relinquish
the CPU from time to time to permit other threads to execute.
A thread can relinquish the CPU in two ways:

* Calling :cpp:func:`k_yield()` puts the thread at the back of the scheduler's
  prioritized list of ready threads, and then invokes the scheduler.
  All ready threads whose priority is higher or equal to that of the
  yielding thread are then allowed to execute before the yielding thread is
  rescheduled. If no such ready threads exist, the scheduler immediately
  reschedules the yielding thread without context switching.

* Calling :cpp:func:`k_sleep()` makes the thread unready for a specified
  time period. Ready threads of *all* priorities are then allowed to execute;
  however, there is no guarantee that threads whose priority is lower
  than that of the sleeping thread will actually be scheduled before
  the sleeping thread becomes ready once again.

Preemptive Time Slicing
=======================

Once a preemptive thread becomes the current thread, it remains
the current thread until a higher priority thread becomes ready,
or until the thread performs an action that makes it unready.
Consequently, if a preemptive thread performs lengthy computations,
it may cause an unacceptable delay in the scheduling of other threads,
including those of equal priority.

To overcome such problems, a preemptive thread can perform cooperative
time slicing (as described above), or the scheduler's time slicing capability
can be used to allow other threads of the same priority to execute.

The scheduler divides time into a series of **time slices**, where slices
are measured in system clock ticks. The time slice size is configurable,
but this size can be changed while the application is running.

At the end of every time slice, the scheduler checks to see if the current
thread is preemptible and, if so, implicitly invokes :cpp:func:`k_yield()`
on behalf of the thread. This gives other ready threads of the same priority
the opportunity to execute before the current thread is scheduled again.
If no threads of equal priority are ready, the current thread remains
the current thread.

Threads with a priority higher than specified limit are exempt from preemptive
time slicing, and are never preempted by a thread of equal priority.
This allows an application to use preemptive time slicing
only when dealing with lower priority threads that are less time-sensitive.

.. note::
   The kernel's time slicing algorithm does *not* ensure that a set
   of equal-priority threads receive an equitable amount of CPU time,
   since it does not measure the amount of time a thread actually gets to
   execute. For example, a thread may become the current thread just before
   the end of a time slice and then immediately have to yield the CPU.
   However, the algorithm *does* ensure that a thread never executes
   for longer than a single time slice without being required to yield.

Scheduler Locking
=================

A preemptible thread that does not wish to be preempted while performing
a critical operation can instruct the scheduler to temporarily treat it
as a cooperative thread by calling :cpp:func:`k_sched_lock()`. This prevents
other threads from interfering while the critical operation is being performed.

Once the critical operation is complete the preemptible thread must call
:cpp:func:`k_sched_unlock()` to restore its normal, preemptible status.

If a thread calls :cpp:func:`k_sched_lock()` and subsequently performs an
action that makes it unready, the scheduler will switch the locking thread out
and allow other threads to execute. When the locking thread again
becomes the current thread, its non-preemptible status is maintained.

.. note::
    Locking out the scheduler is a more efficient way for a preemptible thread
    to inhibit preemption than changing its priority level to a negative value.

.. _metairq_priorities:

Meta-IRQ Priorities
===================

When enabled (see :option:`CONFIG_NUM_METAIRQ_PRIORITIES`), there is a special
subclass of cooperative priorities at the highest (numerically lowest)
end of the priority space: meta-IRQ threads.  These are scheduled
according to their normal priority, but also have the special ability
to preempt all other threads (and other meta-irq threads) at lower
priorities, even if those threads are cooperative and/or have taken a
scheduler lock.

This behavior makes the act of unblocking a meta-IRQ thread (by any
means, e.g. creating it, calling k_sem_give(), etc.) into the
equivalent of a synchronous system call when done by a lower
priority thread, or an ARM-like "pended IRQ" when done from true
interrupt context.  The intent is that this feature will be used to
implement interrupt "bottom half" processing and/or "tasklet" features
in driver subsystems.  The thread, once woken, will be guaranteed to
run before the current CPU returns into application code.

Unlike similar features in other OSes, meta-IRQ threads are true
threads and run on their own stack (which much be allocated normally),
not the per-CPU interrupt stack.  Design work to enable the use of the
IRQ stack on supported architectures is pending.

Note that because this breaks the promise made to cooperative
threads by the Zephyr API (namely that the OS won't schedule other
thread until the current thread deliberately blocks), it should be
used only with great care from application code.  These are not simply
very high priority threads and should not be used as such.

.. _thread_sleeping:

Thread Sleeping
===============

A thread can call :cpp:func:`k_sleep()` to delay its processing
for a specified time period. During the time the thread is sleeping
the CPU is relinquished to allow other ready threads to execute.
Once the specified delay has elapsed the thread becomes ready
and is eligible to be scheduled once again.

A sleeping thread can be woken up prematurely by another thread using
:cpp:func:`k_wakeup()`. This technique can sometimes be used
to permit the secondary thread to signal the sleeping thread
that something has occurred *without* requiring the threads
to define a kernel synchronization object, such as a semaphore.
Waking up a thread that is not sleeping is allowed, but has no effect.

.. _busy_waiting:

Busy Waiting
============

A thread can call :cpp:func:`k_busy_wait()` to perform a ``busy wait``
that delays its processing for a specified time period
*without* relinquishing the CPU to another ready thread.

A busy wait is typically used instead of thread sleeping
when the required delay is too short to warrant having the scheduler
context switch from the current thread to another thread and then back again.

Suggested Uses
**************

Use cooperative threads for device drivers and other performance-critical work.

Use cooperative threads to implement mutually exclusion without the need
for a kernel object, such as a mutex.

Use preemptive threads to give priority to time-sensitive processing
over less time-sensitive processing.

Configuration Options
*********************

Related configuration options:

* :option:`CONFIG_NUM_COOP_PRIORITIES`
* :option:`CONFIG_NUM_PREEMPT_PRIORITIES`
* :option:`CONFIG_TIMESLICING`
* :option:`CONFIG_TIMESLICE_SIZE`
* :option:`CONFIG_TIMESLICE_PRIORITY`

APIs
****

The following thread scheduling-related APIs are provided by :file:`kernel.h`:

* :cpp:func:`k_current_get()`
* :cpp:func:`k_sched_lock()`
* :cpp:func:`k_sched_unlock()`
* :cpp:func:`k_yield()`
* :cpp:func:`k_sleep()`
* :cpp:func:`k_wakeup()`
* :cpp:func:`k_busy_wait()`
* :cpp:func:`k_sched_time_slice_set()`
