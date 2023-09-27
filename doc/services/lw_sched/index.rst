.. _lw_sched_api:

Lightweight (LW) Scheduler
##########################

.. contents::
    :local:
    :depth:  2

The lightweight (LW) scheduler is a dedicated thread that runs at regular
intervals. During each interval, it executes the registered set of LW tasks
in sequence. An LW task is in essence a handler that executes in the context
of the LW scheduler thread.

Within the context of an LW scheduler, an LW task can be in one of three
states: paused, executing and aborted. An executing LW task may have an
associated delay, which merely prevents it from executing further for the
specified number of intervals.

Usage
*****

Initializing the LW Scheduler
=============================

Before the LW scheduler or LW tasks can be used, it must be initialized by
invoking :c:func:`lw_scheduler_init`. This will create the LW scheduler
thread to run at the specified priority with the desired period, but not
start it. Any number of LW schedulers (subject to memory constraints)
may be created. An LW scheduler is started by invoking
:c:func:`lw_scheduler_start`.

.. note::
   Although the LW scheduler will be made ready to execute at regular
   intervals, the scheduling of this thread is still subject to Zephyr's
   thread scheduling algorithm.

The following code creates an LW scheduler to run at thread priority 10 to execute every 50 milliseconds.

.. code-block:: c

    struct lw_scheduler scheduler;
    K_THREAD_STACK_DEFINE(sched_stack, 1024);

    void initialization(void)
    {
        ...
        lw_scheduler_init(&scheduler, sched_stack,
                          K_THREAD_STACK_SIZEOF(sched_stack),
                          K_PRIO_PREEMPT(10), 0, K_MSEC(50));
        ...
    }

Initializing LW Tasks
=====================

After an LW scheduler has been initialized, LW tasks may be both initialized
and registered to it by invoking :c:func:`lw_task_init`. Each LW has its own
set of operations and arguments for the operations. Of these operations, a
handler must be provided for the 'execute' operation, but specifying a handler
for the 'abort' operation is optional. The 'order' parameter is used to manage
the order in which the LW tasks execute, wherein LW tasks with a lower 'order'
are executed before those with a higher 'order'.

LW tasks are initially in the paused state. They are subsequently moved into
the execute state after invoking :c:func:`lw_task_start`.

The following code builds upon the previous sample to create and start a single
LW task.

.. code-block:: c

    struct lw_task task1;

    struct lw_task_ops ops1 = {
        .execute = task1_handler,
        .abort = NULL
    };

    /* Define structures used by LW task's 'execute' handler */

    struct task1_args {
        ...
    };

    struct task1_args execute_args1;

    struct lw_task_args args1 = {
        .execute = &execute_args1,
        .abort = NULL
    }

    void initialization(void)
    {
        ...
        /* Initialize an LW task with order 10 */
        lw_task_init(&task1, &ops1, &args1, &scheduler, 10);
        ...

        /* Start both the LW task and LW scheduler */
        lw_task_start(&task1);
        lw_scheduler_start(&scheduler, K_NO_WAIT);
    }

Delaying LW Tasks
=================

LW tasks can be arbitrarily delayed by invoking :c:func:`lw_task_delay`.
Delays only take effect when the LW task is in the execute state. Although
it is expected that an LW task will delay itself, that does not have to be
the case. Delays are measured in 'intervals', where one interval is the
period of the LW scheduler. Setting the delay to zero will cancel an existing
delay.

The following code builds upon the previous samples to delay the current
LW task.

.. code-block:: c

    void task1_handler(void *arg)
    {
        ...

        /* Delay the current LW task for 15 intervals */

        lw_task_delay(lw_task_current_get(&scheduler), 15);

        ...
    }

Reordering LW Tasks
===================

The order in which tasks execute is not fixed at the time of creation and may
be changed at runtime within certain limitations--any change to the order of
task can only be done while the LW scheduler is idle. That is, if the LW
scheduler is processing its list of tasks, adding new or resorting existing
tasks is delayed until it is finished processing that list. If not, then it
can be done immediately.

The following code builds upon the previous samples to reorder an LW task.

.. code-block:: c

    void readjust(struct lw_task *task)
    {
        ...
        lw_task_reorder(&task1, 42);   /* Change task1's order to 42 */
        ...
    }

Aborting LW Tasks
=================

There are three ways that an LW task can terminate.

The first is that the task can return :c:macro:`LW_TASK_ABORT`. This informs
the scheduler that the task is self-terminating.

The second is if a call to :c:func:`lw_task_abort` is issued to abort the
specified task. If that task is the current task being processed, then it will
terminate once it returns. Otherwise it will be removed immediately.

The third way is if the LW scheduler is aborted using
:c:func:`lw_scheduler_abort`. In this scenario, the LW scheduler thread is
aborted, and then all of its LW tasks are aborted. It is in this scenario
only that the optional abort operation for the task comes into play. If
defined, this handler will be executed. This is a convenient method to
automatically recover system resources.

Suggested Uses
**************

The LW scheduler can be useful to react to a finite state machine and quickly
process a series short handlers in response to various events.

Caveats
*******

It is important to understand that the LW scheduler is built atop the Zephyr
scheduler and care must be taken with regards to blocking calls. Blocking calls
not only block the LW task, but also the LW scheduler itself AND all of its
tasks.

Configuration Options
*********************

Related configuration option:

* :kconfig:option:`CONFIG_LW_SCHEDULER`

API Reference
*************

.. doxygengroup:: subsys_lw_sched
