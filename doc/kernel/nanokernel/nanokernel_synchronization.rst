.. _nanokernel_synchronization:

Synchronization Services
########################

This section describes the synchronization services provided by the nanokernel.
Currently, only a single service is provided.

.. _nanokernel_semaphores:

Nanokernel Semaphores
*********************

Concepts
========

The nanokernel's semaphore object type is an implementation of a traditional
counting semaphore. It is mainly intended for use by fibers.

Any number of nanokernel semaphores can be defined. Each semaphore is a
distinct variable of type :cpp:type:`struct nano_sem`, and is referenced
using a pointer to that variable. A semaphore must be initialized before
it can be used.

A nanokernel semaphore's count is set to zero when the semaphore is initialized.
This count is incremented each time the semaphore is given, and is decremented
each time the semaphore is taken. However, a semaphore cannot be taken if it is
unavailable (i.e. has a count of zero).

A nanokernel semaphore may be given by any context type (i.e. ISR, fiber,
or task).

A nanokernel semaphore may be taken in a non-blocking manner by any
context type; a special return code indicates if the semaphore is unavailable.
A semaphore can also be taken in a blocking manner by a fiber or task;
if the semaphore is unavailable the thread waits for it to be given.

Any number of threads may wait on an unavailable nanokernel semaphore
simultaneously. When the semaphore is signalled it is given to the fiber
that has waited longest, or to a waiting task if no fiber is waiting.

.. note::
   A task that waits on an unavailable nanokernel FIFO semaphore a busy wait.
   This is not an issue for a nanokernel application's background task;
   however, in a microkernel application a task that waits on a nanokernel
   semaphore remains the current task. In contrast, a microkernel task that
   waits on a microkernel synchronization object ceases to be the current task,
   allowing other tasks of equal or lower priority to do useful work.

   If multiple tasks in a microkernel application wait on the same nanokernel
   semaphore, higher priority tasks are given the semaphore in preference to
   lower priority tasks. However, the order in which equal priority tasks
   are given the semaphore is unpredictable.

Purpose
=======

Use a nanokernel semaphore to control access to a set of resources by multiple
fibers.

Use a nanokernel semaphore to synchronize processing between a producing task,
fiber, or ISR and one or more consuming fibers.

Usage
=====

Example: Initializing a Nanokernel Semaphore
--------------------------------------------

This code initializes a nanokernel semaphore, setting its count to zero.

.. code-block:: c

   struct nano_sem input_sem;

   nano_sem_init(&input_sem);

Example: Giving a Nanokernel Semaphore from an ISR
--------------------------------------------------

This code uses a nanokernel semaphore to indicate that a unit of data
is available for processing by a consumer fiber.

.. code-block:: c

   void input_data_interrupt_handler(void *arg)
   {
       /* notify fiber that data is available */
       nano_isr_sem_give(&input_sem);

       ...
   }

Example: Taking a Nanokernel Semaphore with a Conditional Time-out
------------------------------------------------------------------

This code waits up to 500 ticks for a nanokernel semaphore to be given,
and gives warning if it is not obtained in that time.

.. code-block:: c

   void consumer_fiber(void)
   {
       ...

       if (nano_fiber_sem_take_wait_timeout(&input_sem, 500) != 1) {
           printk("Input data not available!");
       } else {
           /* fetch available data */
           ...
       }
       ...
   }

APIs
====

The following APIs for a nanokernel semaphore are provided
by :file:`nanokernel.h`:

:cpp:func:`nano_sem_init()`
   Initializes a semaphore.

:cpp:func:`nano_task_sem_give()`, :cpp:func:`nano_fiber_sem_give()`,
:cpp:func:`nano_isr_sem_give()`, :cpp:func:`nano_sem_give()`
   Signal a sempahore.

:cpp:func:`nano_task_sem_take()`, :cpp:func:`nano_fiber_sem_take()`,
:cpp:func:`nano_isr_sem_take()`
   Test a semaphore.

:cpp:func:`nano_task_sem_take_wait()`,
:cpp:func:`nano_fiber_sem_take_wait()`,
:cpp:func:`nano_sem_task_wait()`
   Wait on a semaphore.

:cpp:func:`nano_task_sem_take_wait_timeout()`,
:cpp:func:`nano_fiber_sem_take_wait_timeout()`
   Wait on a semaphore for a specified time period.