.. _microkernel_semaphores:

Semaphores
##########

Concepts
********

The microkernel's semaphore objects are an implementation of traditional
counting semaphores.

Any number of semaphores can be defined in a microkernel system. Each semaphore
has a name that uniquely identifies it.

A semaphore starts off with a count of zero. This count is incremented each
time the semaphore is given, and is decremented each time the semaphore
is taken. However, a semaphore cannot be taken if it is unavailable
(i.e. has a count of zero).

Semaphores may be given by tasks, fibers, or ISRs.

Semaphores may only be taken by tasks. A task that attempts to take
an unavailable semaphore may choose to wait for the semaphore to be given.
Any number of tasks may wait on an unavailable semaphore simultaneously;
when the semaphore becomes available it is given to the highest priority task
that has waited the longest.

The kernel allows a task to give multiple semaphores in a single
operation using a *semaphore group*. The task specifies the members of
a semaphore group using an array of semaphore names, terminated by the
symbol :c:macro:`ENDLIST`. This technique allows the task to give the semaphores
more efficiently than giving them individually.

A task can also use a semaphore group to take a single semaphore from a set
of semaphores in a single operation. This technique allows the task to
monitor multiple synchronization sources at the same time, similar to the way
:c:func:`select()` can be used to read input from a set of file descriptors
in a POSIX-compliant operating system. The kernel does *not* define the order
in which semaphores are taken when more than one semaphore in a semaphore group
is available; the semaphore that is taken by the task may not be the one
that was given first.

There is no limit on the number of semaphore groups used by a task, or
on the number of semaphores belonging to any given semaphore group. Semaphore
groups may also be shared by multiple tasks, if desired.

Purpose
*******

Use a semaphore to control access to a set of resources by multiple tasks.

Use a semaphore synchronize processing between a producing task, fiber,
or ISR and one or more consuming tasks.

Use a semaphore group to allow a task to signal or to monitor multiple
semaphores simultaneously.

Usage
*****

Defining a Semaphore
====================

The following parameters must be defined:

   *name*
          This specifies a unique name for the semaphore.

Public Semaphore
----------------

Define the semaphore in the application's MDEF using the following syntax:

.. code-block:: console

   SEMA name

For example, the file :file:`projName.mdef` defines two semaphores as follows:

.. code-block:: console

    % SEMA NAME
    % ================
      SEMA INPUT_DATA
      SEMA WORK_DONE

A public semaphore can be referenced by name from any source file that
includes the file :file:`zephyr.h`.

Private Semaphore
-----------------

Define the semaphore in a source file using the following syntax:

.. code-block:: c

   DEFINE_SEMAPHORE(name);

For example, the following code defines a private semaphore named ``PRIV_SEM``.

.. code-block:: c

   DEFINE_SEMAPHORE(PRIV_SEM);

To utilize this semaphore from a different source file use the following syntax:

.. code-block:: c

   extern const ksem_t PRIV_SEM;

Example: Giving a Semaphore from a Task
=======================================

This code uses a semaphore to indicate that a unit of data
is available for processing by a consumer task.

.. code-block:: c

   void producer_task(void)
   {
       /* save data item in a buffer */
       ...

        /* notify task that an additional data item is available */
       task_sem_give(INPUT_DATA);

       ...
   }

Example: Taking a Semaphore with a Conditional Time-out
=======================================================

This code waits up to 500 ticks for a semaphore to be given,
and gives a warning if it is not obtained in that time.

.. code-block:: c

   void consumer_task(void)
   {
       ...

       if (task_sem_take_wait_timeout(INPUT_DATA, 500) == RC_TIME) {
           printf("Input data not available!");
       } else {
           /* extract saved data item from buffer and process it */
           ...
       }
       ...
   }

Example: Monitoring Multiple Semaphores at Once
===============================================

This code waits on two semaphores simultaneously, and then takes
action depending on which one was given.

.. code-block:: c

   ksem_t my_sem_group[3] = { INPUT_DATA, WORK_DONE, ENDLIST };

   void consumer_task(void)
   {
       ksem_t sem_id;
       ...

       sem_id = task_sem_group_take_wait(my_sem_group);
       if (sem_id == WORK_DONE) {
           printf("Shutting down!");
           return;
       } else {
           /* process input data */
           ...
       }
       ...
   }

Example: Giving Multiple Semaphores at Once
===========================================

This code uses a semaphore group to allow a controlling task to signal
the semaphores used by four other tasks in a single operation.

.. code-block:: c

   ksem_t my_sem_group[5] = { SEM1, SEM2, SEM3, SEM4, ENDLIST };

   void control_task(void)
   {
       ...
       task_semaphore_group_give(my_sem_group);
       ...
   }

APIs
****

The following APIs for an individual semaphore are provided by
:file:`microkernel.h`:

:cpp:func:`isr_sem_give()`
   Gives a semaphore (from an ISR).

:cpp:func:`fiber_sem_give()`
   Gives a semaphore (from a fiber).

:cpp:func:`task_sem_give()`
   Gives a semaphore.

:c:func:`task_sem_take()`
   Takes a semaphore, or fails if not given.

:c:func:`task_sem_take_wait()`
   Takes a semaphore, or waits until it is given.

:c:func:`task_sem_take_wait_timeout()`
   Takes a semaphore, or waits for a specified time period.

:cpp:func:`task_sem_reset()`
   Sets the semaphore count to zero.

:cpp:func:`task_sem_count_get()`
   Reads the count for a semaphore.

The following APIs for semaphore groups are provided by microkernel.h.

:cpp:func:`task_sem_group_give()`
   Gives each semaphore in a group.

:c:func:`task_sem_group_take_wait()`
   Takes a semaphore from a group, or waits until one is given.

:c:func:`task_sem_group_take_wait_timeout()`
   Takes a semaphore from a group. or waits for a specified time period.

:cpp:func:`task_sem_group_reset()`
   Sets the count to zero for each semaphore in a group.