.. _tasks:

Task Services
#############

Properties of Tasks
*******************

A task is an execution thread that implements part or all
of the application functionality using the objects described in detail by the
Nanokernel Objects and Microkernel Objects sections. Tasks are cooperatively
scheduled, and will run until they explicitly yield, call a blocking interface
or are preempted by a higher priority task.

Task Groups
***********

TBD (how they are used; maximum of 32 groups; mention pre-defined task groups)

Task Behavior
*************

When a task calls an API to operate on a kernel object, it passes
an abstract object identifier called objectID. A task shall always
manipulate kernel data structures through the APIs and shall not
directly access the internals of any object, for example, the internals
of a semaphore or a FIFO.

Task Implementation
*******************

Use kernel objects and routine calls to interface a task with
other tasks running in the system. For example, achieve cooperation
between tasks by using synchronization objects, such as resources and
semaphores, or by passing parameters from one task to another using a
data-passing object.

Task Stack
==========

The compiler uses the task stack to store local task variables and to
implement parameter-passing between functions. Static and global
variables do not use memory from the stack. For more information about
defining memory segments, and the defaults used for different variable
types, consult the documentation for your compiler.

Task States
===========

Each task has a task state that the scheduler uses to determine whether
it is ready to run. This figure shows the possible task states and the
possible transitions. The most usual transitions are green,
bidirectional transitions are blue and uncommon transitions are marked
orange.

.. figure:: figures/microkernel_tasks_states.svg
   :scale: 75 %
   :alt: Possible Task States

   Shows the possible states that a task might have and their transitions.

Starting and Stopping Tasks
---------------------------

Tasks are started in one of three ways:


+ Automatically at boot time if it is assigned to the EXE task group.
+ Another task issues a :c:func:`task_start()` for the task.
+ Another task issues a :c:func:`task_group_start()` for any task
  group the task belongs to..

The scheduler manages the execution of a task once it is running. If the
task performs a return from the routine that started it, the task
terminates and its stack can be reused. This ensures that the task
terminates safely and cleanly.


Automatically Starting Tasks
----------------------------

Starting tasks automatically at boot utilizes the Task Grouping concept.
The EXE group at boot time will put all tasks belonging to the group in
a runnable state immediately after the kernel boots up.


Tasks Starting Other Tasks
--------------------------

.. todo:: Add details on how to start a task from within another task.

Tasks Scheduling Model
**********************

Once started, a task is scheduled for execution by the microkernel until
one of the following occurs:

* A higher-priority task becomes ready to run.

* The task completes.

* The task's time slice expires and another runnable task of equal
  priority exists.

* The task becomes non-runnable.

Task Completion
===============

.. todo:: Add details on how tasks complete.

Task Priorities
===============

The kernel offers a configurable number of task priority levels. The
number ranges from 0 to :literal:`NUM_TASK_PRIORITIES-1`. The lowest
priority level ( :literal:`NUM_TASK_PRIORITIES-1` is reserved for use
by the microkernel's idle task. The priority of tasks is assigned
during the build process based upon the task definition in the project
file. The priority can be changed at any time, by either the task
itself or by another task calling :c:func:`task_priority_set()`.

If a task of higher priority becomes runnable, the kernel saves the
current tasks context and runs the higher-priority task. It is also
possible for a tasks priority to be temporarily changed to prevent a
condition known as priority inversion.


Priority Preemption
-------------------

The microkernel uses a priority-based preemptive scheduling algorithm
where the highest-priority task that is ready to run, runs. When a task
with a higher priority becomes runnable, the running task is
unscheduled and the task of higher priority is started. This is the
principle of preemption.


Suspended Tasks
===============

Tasks can suspend other tasks, or themselves, using
:c:func:`task_suspend()`. The task stays suspended until
:c:func:`task_resume()` or :c:func:`task_abort()` is called by another
task. Use :c:func:`task_abort()` and :c:func:`task_group_abort()` with
care, as none of the affected tasks may own or be using kernel objects
when they are called. The safest abort practice is for a task to abort
only itself.


Aborting a Task
---------------

Tasks can have an abort handler, C routines that run as a critical
section when a task is aborted. Since the routine runs as critical, it
cannot be preempted or unscheduled allowing the task to properly clean
up. Because of this, abort handlers cannot make kernel API calls.

To install an abort handler function use
:c:func:`task_abort_handler_set()`. This will bind the routine for
execution when :c:func:`task_abort()` is called, and run the abort
handler function immediately.


Task Time-Slicing
=================

Time-slicing, enabled through the :c:func:`sys_scheduler_time_slice_set()`
function, can share a processor between multiple tasks with the same
priority. When enabled, the kernel preempts a task that has run for a
certain amount of time, the time slice, and schedules another runnable
task with the same priority. The sorting of tasks of equal priority
order is a fundamental microkernel scheduling concept and is not
limited to cases involving :c:func:`task_yield()`.

The same effect as time-slicing can be achieved using
:c:func:`task_yield()`. When this call is made, the current task
relinquishes the processor if another task of the same priority is
ready to run. The calling task returns to the queue of runnable tasks.
If no other task of the same priority is runnable, the task that called
:c:func:`task_yield()` continues running.

.. note::

   :c:func:`task_yield()` sorts the tasks in FIFO order.

Task Context Switches
=====================

When a task swap occurs, the kernel saves the context of the task
that is swapped out and restores the context of the task that is
swapped in.

Usage
*****

Defining a Task
===============

The following parameters must be defined:

   *name*
          This specifies a unique name for the task.

   *priority*
          This specifies the scheduling priority of the task. A smaller
          integer value indicates higher priority, with zero indicating
          highest priority.

   *entry_point*
          This specifies the name of the task's entry point function,
          which should have the following form:

          .. code-block:: c

             void <entry_point>(void)".
             {
                 /* task mainline processing */
                 ...
                 /* (optional) normal task termination */
                 return;
             }

   *stack_size*
          This specifies the size of the task's stack, in bytes.

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

.. note::
   Private task groups are not supported by the Zephyr kernel.


Example: Starting a Task from a Different Task
==============================================

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