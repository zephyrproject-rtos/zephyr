.. _nanokernel_synchronization:

Synchronization Services
########################

This section describes the synchronization services provided by the nanokernel.
Currently, only a single service is provided.

Nanokernel Semaphores
*********************

Definition
==========

The nanokernel semaphore is defined in
:file:`kernel/nanokernel/nano_sema.c` and implements a counting
semaphore that sends signals from one fiber to another.

Function
========

Nanokernel semaphore objects can be used from an ISR, a fiber, or the
background task. Interrupt handlers can use the nanokernel’s semaphore
object to reschedule a fiber waiting for the interrupt.

Only one context can wait on a semaphore at a time. The semaphore starts
with a count of 0 and remains that way if no context is pending on it.
Each 'give' operation increments the count by 1. Following multiple
'give' operations, the same number of 'take' operations can be
performed without the calling context having to wait on the semaphore.
Thus after n 'give' operations a semaphore can 'take' n times without
pending. If a second context waits for the same semaphore object, the
first context is lost and never wakes up.

APIs
====

The following APIs for a nanokernel semaphore are provided
by :file:`nanokernel.h.`

+------------------------------------------------+----------------------------+
| Call                                           | Description                |
+================================================+============================+
| :c:func:`nano_sem_init()`                      | Initializes a semaphore.   |
+------------------------------------------------+----------------------------+
| | :c:func:`nano_task_sem_give()`               | Signals a sempahore.       |
| | :c:func:`nano_fiber_sem_give()`              |                            |
| | :c:func:`nano_isr_sem_give()`                |                            |
| | :c:func:`nano_sem_give()`                    |                            |
+------------------------------------------------+----------------------------+
| | :c:func:`nano_task_sem_take()`               | Tests a semaphore.         |
| | :c:func:`nano_fiber_sem_take()`              |                            |
| | :c:func:`nano_isr_sem_take()`                |                            |
+------------------------------------------------+----------------------------+
| | :c:func:`nano_task_sem_take_wait()`          | Waits on a semaphore.      |
| | :c:func:`nano_fiber_sem_take_wait()`         |                            |
| | :c:func:`nano_sem_task_wait()`               |                            |
+------------------------------------------------+----------------------------+
| | :c:func:`nano_task_sem_take_wait_timeout()`  | Waits on a semaphore for a |
| | :c:func:`nano_fiber_sem_take_wait_timeout()` | specified time period.     |
+------------------------------------------------+----------------------------+
