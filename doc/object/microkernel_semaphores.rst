.. _microkernel_semaphores:

Semaphores
**********

Definition
==========

The microkernel semaphore is defined in
:file:`kernel/microkernel/k_semaphore.c` and are an implementation of
traditional counting semaphores. Semaphores are used to synchronize
application task activities.

Function
========

Semaphores are initialized by the system. At start the semaphore is
un-signaled and no task is waiting for it. Any task in the system can
signal a semaphore. Every signal increments the count value associated
with the semaphore. When several tasks wait for the same semaphore at
the same time, they are held in a prioritized list. If the semaphore is
signaled, the task with the highest priority is released. If more tasks
of that priority are waiting, the first one that requested the
semaphore wakes up. Other tasks can test the semaphore to see if it is
signaled. If not signaled, tasks can either wait, with or without a
timeout, until signaled or return immediately with a failed status.

Initialization
==============

A semaphore has to be defined in the project file, for example
:file:`projName.mdef`, which will specify the object type, and the name
of the semaphore. Use the following syntax in the MDEF file to define a
semaphore::

.. code-block:: console

   SEMA %name %node

An example of a semaphore entry for use in the MDEF file:

.. code-block:: console

   % SEMA   NAME

   % =================

     SEMA   SEM_TASKDONE



Application Program Interfaces
==============================

Semaphore APIs allow signaling a semaphore. They also provide means to
reset the signal count.

+----------------------------------------+------------------------------------+
| Call                                   | Description                        |
+========================================+====================================+
| :c:func:`isr_sem_give()`               | Signal a semaphore from an ISR.    |
+----------------------------------------+------------------------------------+
| :c:func:`task_sem_give()`              | Signal a semaphore from a task.    |
+----------------------------------------+------------------------------------+
| :c:func:`task_sem_take()`              | Test a semaphore from a task.      |
+----------------------------------------+------------------------------------+
| :c:func:`task_sem_take_wait()`         | Wait on a semaphore from a task.   |
+----------------------------------------+------------------------------------+
| :c:func:`task_sem_take_wait_timeout()` | Wait on a semaphore, with a        |
|                                        | timeout, from a task.              |
+----------------------------------------+------------------------------------+
| :c:func:`task_sem_group_reset()`       | Sets a list of semaphores to zero. |
+----------------------------------------+------------------------------------+
| :c:func:`task_sem_group_give()`        | Signals a list of semaphores from  |
|                                        | a task.                            |
+----------------------------------------+------------------------------------+
| :c:func:`task_sem_reset()`             | Sets a semaphore to zero.          |
+----------------------------------------+------------------------------------+
