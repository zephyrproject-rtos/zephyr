.. _nanokernel_signaling:

Signaling Services
##################

Nanokernel Semaphore
********************


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

Application Program Interfaces
==============================

+--------------------------------+----------------------------------------------------------------+
| Context                        | Interfaces                                                     |
+================================+================================================================+
| **Initialization**             | :c:func:`nano_sem_init()`                                      |
+--------------------------------+----------------------------------------------------------------+
| **Interrupt Service Routines** | :c:func:`nano_isr_sem_give()`,                                 |
|                                | :c:func:`nano_isr_sem_take()`                                  |
+--------------------------------+----------------------------------------------------------------+
| **Fibers**                     | :c:func:`nano_fiber_sem_give()`,                               |
|                                | :c:func:`nano_fiber_sem_take()`,                               |
|                                | :c:func:`nano_fiber_sem_take_wait()`                           |
+--------------------------------+----------------------------------------------------------------+
| **Tasks**                      | :c:func:`nano_task_sem_give()`,                                |
|                                | :c:func:`nano_task_sem_take()`,                                |
|                                | :c:func:`nano_task_sem_take_wait()`                            |
+--------------------------------+----------------------------------------------------------------+