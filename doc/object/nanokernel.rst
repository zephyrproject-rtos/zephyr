.. _nanokernelObjects:

Nanokernel Objects
##################

Section Scope
*************

This section provides an overview of the most important nanokernel
objects. The information contained here is an aid to better understand
how the Zephyr Kernel operates at the nanokernel level.

Document Format
***************


Each object is broken off to its own section, containing a definition, a
functional description, the object initialization syntax, and a table
of Application Program Interfaces (APIs) with the context which may
call them. Please refer to the API documentation for further details
regarding each object’s functionality.

Nanokernel FIFO
***************

Definition
==========


The FIFO object is defined in :file:`kernel/nanokernel/nano_fifo.c`.
This is a linked list of memory that allows the caller to store data of
any size. The data is stored in first-in-first-out order.

Function
========


Multiple processes can wait on the same FIFO object. Data is passed to
the first fiber that waited on the FIFO, and then to the background
task if no fibers are waiting. Through this mechanism the FIFO object
can synchronize or communicate between more than two contexts through
its API. Any ISR, fiber, or task can attempt to get data from a FIFO
without waiting on the data to be stored.

.. note::

   The FIFO object reserves the first WORD in each stored memory
   block as a link pointer to the next item. The size of the WORD
   depends on the platform and can be 16 bit, 32 bit, etc.

Application Program Interfaces
==============================

+--------------------------------+--------------------------------------------------------------------------------------------------------+
| **Context**                    | **Interfaces**                                                                                         |
+--------------------------------+--------------------------------------------------------------------------------------------------------+
| **Initialization**             | :c:func:`nano_fifo_init()`                                                                             |
+--------------------------------+--------------------------------------------------------------------------------------------------------+
| **Interrupt Service Routines** | :c:func:`nano_isr_fifo_get()`, :c:func:`nano_isr_fifo_put()`                                           |
+--------------------------------+--------------------------------------------------------------------------------------------------------+
| **Fibers**                     | :c:func:`nano_fiber_fifo_get()`, :c:func:`nano_fiber_fifo_get_wait()`, :c:func:`nano_fiber_fifo_put()` |
+--------------------------------+--------------------------------------------------------------------------------------------------------+
| **Tasks**                      | :c:func:`nano_task_fifo_get()`, :c:func:`nano_task_fifo_get_wait()`, :c:func:`nano_task_fifo_put()`    |
+--------------------------------+--------------------------------------------------------------------------------------------------------+

Nanokernel LIFO Object
**********************

Definition
==========

The LIFO is defined in :file:`kernel/nanokernel/nano_lifo.c`. It
consists of a linked list of memory blocks that uses the first word in
each block as a next pointer. The data is stored in last-in-first-out
order.

Function
========

When a message is added to the LIFO, the data is stored at the head of
the list. Messages taken off the LIFO object are taken from the head.

The LIFO object requires the first 32-bit word to be empty in order to
maintain the linked list.

The LIFO object does not store information about the size of the
messages.

The LIFO object remembers one waiting context. When a second context
starts waiting for data from the same LIFO object, the first context
remains waiting and never reaches the runnable state.

Application Program Interfaces
==============================

+--------------------------------+--------------------------------------------------------------------------------------------------------+
| **Context**                    | **Interfaces**                                                                                         |
+--------------------------------+--------------------------------------------------------------------------------------------------------+
| **Initialization**             | :c:func:`nano_lifo_init()`                                                                             |
+--------------------------------+--------------------------------------------------------------------------------------------------------+
| **Interrupt Service Routines** | :c:func:`nano_isr_lifo_get()`, :c:func:`nano_isr_lifo_put()`                                           |
+--------------------------------+--------------------------------------------------------------------------------------------------------+
| **Fibers**                     | :c:func:`nano_fiber_lifo_get()`, :c:func:`nano_fiber_lifo_get_wait()`, :c:func:`nano_fiber_lifo_put()` |
+--------------------------------+--------------------------------------------------------------------------------------------------------+
| **Tasks**                      | :c:func:`nano_task_lifo_get()`, :c:func:`nano_task_lifo_get_wait()`, :c:func:`nano_task_lifo_put()`    |
+--------------------------------+--------------------------------------------------------------------------------------------------------+

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

+--------------------------------+--------------------------------------------------------------------------------------------------------+
| Context                        | Interfaces                                                                                             |
+================================+========================================================================================================+
| **Initialization**             | :c:func:`nano_sem_init()`                                                                              |
+--------------------------------+--------------------------------------------------------------------------------------------------------+
| **Interrupt Service Routines** | :c:func:`nano_isr_sem_give()`, :c:func:`nano_isr_sem_take()`                                           |
+--------------------------------+--------------------------------------------------------------------------------------------------------+
| **Fibers**                     | :c:func:`nano_fiber_sem_give()`, :c:func:`nano_fiber_sem_take()`, :c:func:`nano_fiber_sem_take_wait()` |
+--------------------------------+--------------------------------------------------------------------------------------------------------+
| **Tasks**                      | :c:func:`nano_task_sem_give()`, :c:func:`nano_task_sem_take()`, :c:func:`nano_task_sem_take_wait()`    |
+--------------------------------+--------------------------------------------------------------------------------------------------------+

Timer Objects
*************

Definition
==========

The timer objects is defined in :file:`kernel/nanokernel/nano_timer.c`
and implements digital counters that either increment or decrement at a
fixed frequency. Timers can be called from a task or fiber context.

Function
========

Only a fiber or task context can call timers. Timers can only be used in
a nanokernel if it is not part of a microkernel. Timers are optional in
nanokernel-only systems. The nanokernel timers are simple. The
:c:func:`nano_node_tick_delta()` routine is not reentrant and should
only be called from a single context, unless it is certain other
contexts are not using the elapsed timer.


Application Program Interfaces
==============================

+--------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------+
| **Context**                    | **Interface**                                                                                                                               |
+--------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------+
| **Initialization**             | :c:func:`nano_timer_init()`                                                                                                                 |
+--------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------+
| **Interrupt Service Routines** |                                                                                                                                             |
+--------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------+
| **Fibers**                     | :c:func:`nano_fiber_timer_test()`, :c:func:`nano_fiber_timer_wait()`, :c:func:`nano_fiber_timer_start()`, :c:func:`nano_fiber_timer_stop()` |
+--------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------+
| **Tasks**                      | :c:func:`nano_task_timer_test()`, :c:func:`nano_task_timer_wait()`, :c:func:`nano_task_timer_start()`, :c:func:`nano_task_timer_stop()`     |
+--------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------+

Semaphore, Timer, and Fiber Example
***********************************


The following example is pulled from the file:
:file:`samples/microkernel/apps/hello_world/src/hello.c`.

Example Code
============

.. code-block:: c

   #include <nanokernel.h>

   #include <nanokernel/cpu.h>

   /* specify delay between greetings (in ms); compute equivalent in ticks */

   #define SLEEPTIME

   #define SLEEPTICKS (SLEEPTIME * CONFIG_TICKFREQ / 1000)

   #define STACKSIZE 2000

   char fiberStack[STACKSIZE];

   struct nano_sem nanoSemTask;

   struct nano_sem nanoSemFiber;

   void fiberEntry (void)

   {
      struct nano_timer timer;
      uint32_t data[2] = {0, 0};

      nano_sem_init (&nanoSemFiber);
      nano_timer_init (&timer, data);

      while (1)
      {
         /* wait for task to let us have a turn */
         nano_fiber_sem_take_wait (&nanoSemFiber);

         /* say "hello" */
         PRINT ("%s: Hello World!\n", __FUNCTION__);

         /* wait a while, then let task have a turn */
         nano_fiber_timer_start (&timer, SLEEPTICKS);
         nano_fiber_timer_wait (&timer);
         nano_fiber_sem_give (&nanoSemTask);
      }

   }

   void main (void)

   {
      struct nano_timer timer;
      uint32_t data[2] = {0, 0};

      task_fiber_start (&fiberStack[0], STACKSIZE,
                     (nano_fiber_entry_t) fiberEntry, 0, 0, 7, 0);

      nano_sem_init (&nanoSemTask);
      nano_timer_init (&timer, data);

      while (1)
      {
         /* say "hello" */
         PRINT ("%s: Hello World!\n", __FUNCTION__);

         /* wait a while, then let fiber have a turn */
         nano_task_timer_start (&timer, SLEEPTICKS);
         nano_task_timer_wait (&timer);
         nano_task_sem_give (&nanoSemFiber);

         /* now wait for fiber to let us have a turn */
         nano_task_sem_take_wait (&nanoSemTask);
      }

   }

Step-by-Step Description
========================

A quick breakdown of the major objects in use by this sample includes:

- One fiber, executing in the :c:func:`fiberEntry()` routine.

- The background task, executing in the :c:func:`main()` routine.

- Two semaphores (*nanoSemTask*, *nanoSemFiber*),

- Two timers:

   + One local to the fiber (timer)

   + One local to background task (timer)

First, the background task starts executing main(). The background task
calls task_fiber_start initializing and starting the fiber. Since a
fiber is available to be run, the background task is pre-empted and the
fiber begins running.

Execution jumps to fiberEntry. nanoSemFiber and the fiber-local timer
before dropping into the while loop, where it takes and waits on
nanoSemFiber. task_fiber_start.

The background task initializes nanoSemTask and the task-local timer.

The following steps repeat endlessly:

#. The background task execution begins at the top of the main while
   loop and prints, “main: Hello World!”

#. The background task then starts a timer for SLEEPTICKS in the
   future, and waits for that timer to expire.


#. Once the timer expires, it signals the fiber by giving the
   nanoSemFiber semaphore, which in turn marks the fiber as runnable.

#. The fiber, now marked as runnable, pre-empts the background
   process, allowing execution to jump to the fiber.
   nano_fiber_sem_take_wait.

#. The fiber then prints, “fiberEntry: Hello World!” It starts a time
   for SLEEPTICKS in the future and waits for that timer to expire. The
   fiber is marked as not runnable, and execution jumps to the
   background task.

#. The background task then takes and waits on the nanoSemTask
   semaphore.

#. Once the timer expires, the fiber signals the background task by
   giving the nanoSemFiber semaphore. The background task is marked as
   runnable, but code execution continues in the fiber, since fibers
   take priority over the background task. The fiber execution
   continues to the top of the while loop, where it takes and waits on
   nanoSemFiber. The fiber is marked as not runnable, and the
   background task is scheduled.

#. The background task execution picks up after the call to
   :c:func:`nano_task_sem_take_wait()`. It jumps to the top of the
   while loop.
