.. _microkernel_tasks:

Task Services
#############

Concepts
********

A task is a preemptible thread of execution that implements a portion of
an application's processing. It is is normally used to perform processing that
is too lengthy or complex to be performed by a fiber or an ISR.

A microkernel application can define any number of application tasks. Each
task has a name that uniquely identifies it. Other important task properties
that must be specified include:

* a memory region that is used for its stack and for other execution context
  information,
* a function that is invoked when the task starts executing, and
* a priority that is used by the microkernel scheduler.

The microkernel automatically defines a system task, known as the *idle task*,
which has lowest priority. This task is used during system initialization,
and subsequently executes only when there is no other work for the system to do.
The idle task is anonymous and must not be referenced by application tasks.

.. note::
   A nanokernel application can define only a single application task, known
   as the *background task*, which is very different from the microkernel tasks
   described in this section. For more information see
   :ref:`Nanokernel Task Services <nanokernel_tasks>`.

Task Lifecycle
==============

The kernel automatically starts a task during system initialization if the task
belongs to the :c:macro:`EXE` task group; see `Task Groups`_ below.
A task that is not started automatically must be started by another task
using :c:func:`task_start()`.

Once a task is started it normally executes forever. A task may terminate
gracefully by simply returning from its entry point function. If it does,
it is the task's responsibility to release any system resources it may own
(such as mutexes and dynamically allocated memory blocks) prior to returning,
since the kernel does *not* attempt to reclaim them so they can be reused.

A task may also terminate non-gracefully by *aborting*. The kernel
automatically aborts a task when it generates a fatal error condition,
such as dereferencing a null pointer. A task can also be explicitly aborted
using :c:func:`task_abort()`. As with graceful task termination,
the kernel does not attempt to reclaim system resources owned by the task.

A task may optionally register an *abort handler function* that is invoked
by the kernel when the task terminates (including during graceful termination).
The abort handler can be used to record information about the terminating
task or to assist in reclaiming system resources owned by the task. The abort
handler function is invoked by the microkernel server fiber, so it cannot
directly call kernel APIs that must be invoked by a task; instead, it must
co-ordindate with another task to invoke such APIs indirectly.

.. note::
   The kernel does not currently make any claims regarding an application's
   ability to restart a terminated task.


Task Scheduling
===============

The microkernel's scheduler selects which of the system's tasks is allowed
to execute; this task is known as the *current task*. The nanokernel's scheduler
permits the current task to execute only when there is no fiber or ISR
that needs to execute, since fiber and ISR execution takes precedence.

The kernel automatically takes care of saving the current task's CPU register
values when it performs a context switch to a different task, a fiber, or
an ISR, and restores these values when the task later resumes execution.

Task State
----------

A microkernel task has an associated *state* that determines whether or not
it can be scheduled for execution. The state records all factors that can
prevent the task from executing, such as:

* the task has not been started
* the task is waiting for it needs (e.g. a semaphore, a timeout, ...)
* the task has been suspended
* the task has terminated

A task whose state has no factors that prevent its execution is said to be
*executable*.

Task Priorities
---------------

A microkernel application can be configured to support any number of task
priority levels using the :option:`NUM_TASK_PRIORITIES` configuration option.

An application task can have any priority from 0 (highest priority) down to
:option:`NUM_TASK_PRIORITIES`-2. The lowest priority level,
:option:`NUM_TASK_PRIORITIES`-1, is reserved for the microkernel's idle task.

A task's original priority can be altered up or down after the task has been
started.

Scheduling Algorithm
--------------------

The microkernel's scheduler always selects the highest priority executable task
to be the current task. If multiple executable tasks of that priority
are available the scheduler chooses the one that has been waiting longest.

Once a task becomes the current task it remains scheduled for execution
by the microkernel until one of the following occurs:

* The task is supplanted by a higher priority task that becomes ready to
  execute.

* The task is supplanted by an equal priority task that is ready to execute,
  either because the current task explicitly calls :c:func:`task_yield()`
  or because the kernel implicitly calls :c:func:`task_yield()` because the
  scheduler's time slice has expired.

* The task is supplanted by an equal or lower priority task that is ready
  to execute, because the current task calls a kernel API that blocks its
  own execution. (For example, the task attempts to take a semaphore that
  is unavailable.)

* The task terminates itself by returning from its entry point function.

* The task aborts itself by performing an operation that causes a fatal error.

Time Slicing
------------

The microkernel's scheduler supports an optional time slicing capability
that prevents a task from monopolizing the CPU when other tasks of the
same priority are ready to execute.

The scheduler divides time into a series of *time slices*, whose size is
measured in system clock ticks. The time slice size is specified by
the :option:`TIMESLICE_SIZE` configuration option, but this size can also
be changed dynamically while the application is running.

At the end of every time slice the scheduler implicitly invokes
:c:func:`task_yield()` on behalf of the current task, thereby giving
all other tasks of that priority the opportunity to execute before the
current task can once again be scheduled. If one or more equal priority
tasks are ready to execute, the current task is preempted to allow those
tasks to execute. If no equal priority tasks are ready to execute,
the current task remains the current task, and continues to execute.

Tasks having a priority higher than that specified by the
:option:`TIMESLICE_PRIORITY` configuration option are exempt from time
slicing, and are never preempted by a task of equal priority. This
capability allows an application to use time slicing only for lower
priority tasks that are less time-sensitive.

.. note::
   The microkernel's time slicing algorithm does *not* ensure that a set
   of equal priority tasks will receive an equitable amount of CPU time,
   since it does not measure the amount of time a task actually gets to
   execute. For example, a task may become the current task just before
   the end of a time slice and then immediately have to yield the CPU.
   On the other hand, the microkernel's scheduler *does* ensure that a task
   never executes for longer than a single time slice without being required
   to yield.

Task Suspension
---------------

The microkernel allows a task to be *suspended*, which prevents the task
from executing for an indefinite period of time. The :c:func:`task_suspend()`
API allows an application task to suspend any other task, including itself.
Suspending a task that is already suspended has no additional effect.

Once suspended, a task cannot be scheduled until another task calls
:c:func:`task_resume()` to remove the suspension.

.. note::
   A task can prevent itself from executing for a specified period of time
   using :c:func:`task_sleep()`. However, this is different from suspending
   a task since a sleeping task becomes executable automatically when the
   time limit is reached.

Task Groups
===========

The kernel allows a set of related tasks, known as a *task group*, to be
manipulated as a single unit, rather than individually. This simplifies
the work required to start related tasks, to suspend and resume them, or
to abort them.

A task can be defined so that it initially belongs to a single task group,
multiple task groups, or no task group. A task's group memberships can
also be changed dynamically while the application is running.

The kernel supports a maximum of 32 distinct task groups. The task groups
listed below are pre-defined by the kernel; additional task groups can
be defined by the application.

   :c:macro:`EXE`
      The set of tasks which are started automatically by the kernel
      during system intialization.

   :c:macro:`SYS`
      The set of system tasks which continue executing during system debugging.

   :c:macro:`FPU`
      The set of tasks that require the kernel to save x87 FPU and MMX floating
      point context information during context switches.

   :c:macro:`SSE`
      The set of tasks that require the kernel to save SSE floating point
      context information during context switches. (Tasks in this group are
      implicitly members of the :c:macro:`FPU` task group too.)


Usage
*****

Defining a Task
===============

The following parameters must be defined:

   *name*
          This specifies a unique name for the task.

   *priority*
          This specifies the scheduling priority of the task.

   *entry_point*
          This specifies the name of the task's entry point function,
          which should have the following form:

          .. code-block:: c

             void <entry_point>(void)
             {
                 /* task mainline processing */
                 ...
                 /* (optional) normal task termination */
                 return;
             }

   *stack_size*
          This specifies the size of the memory region used for the task's
          stack and for other execution context information, in bytes.

   *groups*
          This specifies the task groups the task belongs to.

Public Task
-----------

Define the task in the application's .MDEF file using the following syntax:

.. code-block:: console

   TASK name priority entry_point stack_size groups

The task groups are specified using a comma-separated list of task group names
enclosed in square brackets, with no embedded spaces. If the task does not
belong to any task group specify an empty list; i.e. :literal:`[]`.

For example, the file :file:`projName.mdef` defines a system comprised
of six tasks as follows:

.. code-block:: console

   % TASK NAME           PRIO  ENTRY          STACK   GROUPS
   % ===================================================================
     TASK MAIN_TASK        6   keypad_main     1024   [KEYPAD_TASKS,EXE]
     TASK PROBE_TASK       2   probe_main       400   []
     TASK SCREEN1_TASK     8   screen_1_main   4096   [VIDEO_TASKS]
     TASK SCREEN2_TASK     8   screen_2_main   4096   [VIDEO_TASKS]
     TASK SPEAKER1_TASK   10   speaker_1_main  1024   [AUDIO_TASKS]
     TASK SPEAKER2_TASK   10   speaker_2_main  1024   [AUDIO_TASKS]

A public task can be referenced from any source file that includes
the file :file:`zephyr.h`.


Private Task
------------

Define the task in a source file using the following syntax:

.. code-block:: c

   DEFINE_TASK(PRIV_TASK, priority, entry, stack_size, groups);

The task groups are specified using a list of task group names separated by
:literal:`|`; i.e. the logical OR operator. If the task does not belong to any
task group specify NULL.

For example, the following code can be used to define a private task named
``PRIV_TASK``.

.. code-block:: c

   DEFINE_TASK(PRIV_TASK, 10, priv_task_main, 800, EXE);

To utilize this task from a different source file use the following syntax:

.. code-block:: c

   extern const ktask_t PRIV_TASK;


Defining a Task Group
=====================

The following parameters must be defined:

   *name*
          This specifies a unique name for the task group.

Public Task Group
-----------------

Define the task group in the application's .MDEF file using the following
syntax:

.. code-block:: console

   TASKGROUP name

For example, the file :file:`projName.mdef` defines three new task groups
as follows:

.. code-block:: console

   % TASKGROUP   NAME
   % ========================
     TASKGROUP   VIDEO_TASKS
     TASKGROUP   AUDIO_TASKS
     TASKGROUP   KEYPAD_TASKS

A public task group can be referenced from any source file that includes
the file :file:`zephyr.h`.

.. note::
   Private task groups are not supported by the Zephyr kernel.


Example: Starting a Task from Another Task
==========================================

This code shows how the currently executing task can start another task.

.. code-block:: c

   void keypad_main(void)
   {
       /* begin system initialization */
       ...

       /* start task to monitor temperature */
       task_start(PROBE_TASK);

       /* continue to bring up and operate system */
       ...
   }


Example: Suspending and Resuming a Set of Tasks
===============================================

This code shows how the currently executing task can temporarily suspend
the execution of all tasks belonging to the designated task groups.

.. code-block:: c

   void probe_main(void)
   {
       int was_overheated = 0;

       /* continuously monitor temperature */
       while (1) {
           now_overheated = overheating_update();

           /* suspend non-essential tasks when overheating is detected */
           if (now_overheated && !was_overheated) {
              task_group_suspend(VIDEO_TASKS | AUDIO_TASKS);
              was_overheated = 1;
           }

           /* resume non-essential tasks when overheating abates */
           if (!now_overheated && was_overheated) {
              task_group_resume(VIDEO_TASKS | AUDIO_TASKS);
              was_overheated = 0;
           }

           /* wait 10 ticks of system clock before checking again */
           task_sleep(10);
       }
   }

Example: Offloading Work to the Microkernel Server Fiber
========================================================

This code shows how the currently executing task can perform critical section
processing by offloading it to the microkernel server. Since the critical
section function is being executed by a fiber, once the function begins
executing it cannot be interrupted by any other fiber or task that wants
to log an alarm.

.. code-block:: c

   /* alarm logging subsystem */

   #define MAX_ALARMS 100

   struct alarm_info alarm_log[MAX_ALARMS];
   int num_alarms = 0;

   int log_an_alarm(struct alarm_info *new_alarm)
   {
       /* ensure alarm log isn't full */
       if (num_alarms == MAX_ALARMS) {
           return 0;
       }

       /* add new alarm to alarm log */
       alarm_info[num_alarms] = *new_alarm;
       num_alarms++;

       /* pass back alarm identifier to indicate successful logging */
       return num_alarms;
   }

   /* task that generates an alarm */

   void XXX_main(void)
   {
       struct alarm_info my_alarm = { ... };

       ...

       /* record alarm in system's database */
       if (task_offload_to_fiber(log_an_alarm, &my_alarm) == 0) {
           printf("Unable to log alarm!");
       }

       ...
   }

APIs
****

The following APIs affecting the currently executing task
are provided by :file:`microkernel.h`.

+-------------------------------------+-----------------------------------------+
| Call                                | Description                             |
+=====================================+=========================================+
| :c:func:`task_id_get()`             | Gets the task's ID.                     |
+-------------------------------------+-----------------------------------------+
| :c:func:`isr_task_id_get()`         | Gets the task's ID from an ISR.         |
+-------------------------------------+-----------------------------------------+
| :c:func:`task_priority_get()`       | Gets the task's priority.               |
+-------------------------------------+-----------------------------------------+
| :c:func:`isr_task_priority_get()`   | Gets the task's priority from an ISR.   |
+-------------------------------------+-----------------------------------------+
| :c:func:`task_group_mask_get()`     | Gets the task's group memberships.      |
+-------------------------------------+-----------------------------------------+
| :c:func:`isr_task_group_mask_get()` | Gets the task's group memberships from  |
|                                     | an ISR.                                 |
+-------------------------------------+-----------------------------------------+
| :c:func:`task_abort_handler_set()`  | Installs the task's abort handler.      |
+-------------------------------------+-----------------------------------------+
| :c:func:`task_yield()`              | Yields CPU to equal-priority tasks.     |
+-------------------------------------+-----------------------------------------+
| :c:func:`task_sleep()`              | Yields CPU for a specified time period. |
+-------------------------------------+-----------------------------------------+
| :c:func:`task_offload_to_fiber()`   | Instructs the microkernel server fiber  |
|                                     | to execute a function.                  |
+-------------------------------------+-----------------------------------------+

The following APIs affecting a specified task
are provided by :file:`microkernel.h`.

+-------------------------------------------+----------------------------------+
| Call                                      | Description                      |
+===========================================+==================================+
| :c:func:`task_priority_set()`             | Sets a task's priority.          |
+-------------------------------------------+----------------------------------+
| :c:func:`task_entry_set()`                | Sets a task's entry point.       |
+-------------------------------------------+----------------------------------+
| :c:func:`task_start()`                    | Starts execution of a task.      |
+-------------------------------------------+----------------------------------+
| :c:func:`task_suspend()`                  | Suspends execution of a task.    |
+-------------------------------------------+----------------------------------+
| :c:func:`task_resume()`                   | Resumes execution of a task.     |
+-------------------------------------------+----------------------------------+
| :c:func:`task_abort()`                    | Aborts execution of a task.      |
+-------------------------------------------+----------------------------------+
| :c:func:`task_group_join()`               | Adds a task to the specified     |
|                                           | task group(s).                   |
+-------------------------------------------+----------------------------------+
| :c:func:`task_group_leave()`              | Removes a task from the          |
|                                           | specified task group(s).         |
+-------------------------------------------+----------------------------------+

The following APIs affecting multiple tasks
are provided by :file:`microkernel.h`.

+-------------------------------------------+---------------------------------+
| Call                                      | Description                     |
+===========================================+=================================+
| :c:func:`sys_scheduler_time_slice_set()`  | Sets the time slice period used |
|                                           | in round-robin task scheduling. |
+-------------------------------------------+---------------------------------+
| :c:func:`task_group_start()`              | Starts execution of all tasks   |
|                                           | in the specified task groups.   |
+-------------------------------------------+---------------------------------+
| :c:func:`task_group_suspend()`            | Suspends execution of all tasks |
|                                           | in the specified task groups.   |
+-------------------------------------------+---------------------------------+
| :c:func:`task_group_resume()`             | Resumes execution of all tasks  |
|                                           | in the specified task groups.   |
+-------------------------------------------+---------------------------------+
| :c:func:`task_group_abort()`              | Aborts execution of all tasks   |
|                                           | in the specified task groups.   |
+-------------------------------------------+---------------------------------+