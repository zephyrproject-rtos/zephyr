.. _nanokernel_timers:

Timer Services
##############

Concepts
********

A nanokernel timer uses the kernel's system clock to monitor the passage
of time, as measured in ticks. A timer allows a fiber or task to determine
if a specified time limit has been reached while the thread itself is busy
performing other work.

Any number of nanokernel timers can be defined. Each timer is uniquely
identified by the memory address of its associated timer data structure.
A thread can use more than one timer when it needs to monitor multiple time
intervals simultaneously.

Before a timer can be used it must first be initialized with the pointer
to a word-aligned *user data structure* of at least 4 bytes in size.
The first 4 bytes of the user data structure are reserved for kernel use;
any remaining bytes can be used to hold data that may be helpful
to the thread that uses the timer.

A timer is started by specifying a duration, which is the number of ticks
the timer counts before it expires.

.. note::
   Care must be taken when specifying the duration of a nanokernel timer,
   since the first tick measured by the timer after it is started will be
   less than a full tick interval. For example, when the system clock period
   is 10 milliseconds starting a timer than expires after 1 tick will result
   in the timer expiring anywhere from a fraction of a millisecond
   later to just slightly less than 10 milliseconds later. To ensure that
   a timer doesn't expire for at least N ticks it is necessary to specify
   a duration of N+1 ticks.

Once started, a timer can be tested in either a non-blocking or blocking
manner to allow a thread to determine if the timer has expired. If the timer
has expired the kernel returns the pointer to the user data structure.
If the timer has not expired the kernel either returns NULL (for a
non-blocking test) or waits for the timer to expire (for a blocking test).

.. note::
   The nanokernel does not allow more than one thread to wait on a timer
   at any given time. If a second thread starts waiting only the first
   waiting thread wakes up when the timer expires and the second thread
   continues waiting.

A timer can be cancelled after it has been started. Cancelling a timer
while it is still running causes the timer to expire immediately,
thereby unblocking any thread waiting on the timer. Cancelling a timer
that has already expired has no effect on the timer.

A timer can be reused once it has expired, but must **not** be restarted
while it is still running. If desired, a timer can be re-initialized
with a different user data structure before it is started again.


Purpose
*******

Use a nanokernel timer to determine whether or not a specified number
of system clock ticks have elapsed while a fiber or task is busy performing
other work.

.. note::
   If a fiber or task has no other work to perform while waiting
   for time to pass it can simply call :c:func:`fiber_sleep()`
   or :c:func:`task_sleep()`, respectively.

.. note::
   The kernel provides additional APIs that allow a fiber or task to monitor
   the system clock, as well as the higher precision hardware clock,
   without using a nanokernel timer.


Example: Initializing a Nanokernel Timer
========================================

This code initializes a timer.

.. code-block:: c

   struct nano_timer my_timer;
   uint32_t data_area[3] = { 0, 1111, 2222 };

   nano_timer_init(&my_timer, data_area);


Example: Starting a Nanokernel Timer
====================================
This code uses the above timer to limit the amount of time a fiber
spends gathering data before processing it.

.. code-block:: c

   /* set timer to expire in 10 ticks */
   nano_fiber_timer_start(&my_timer, 10);

   /* gather data until timer expires */
   do {
       ...
   } while (nano_fiber_timer_test(&my_timer) == NULL);

   /* process the data */
   ...


Example: Cancelling a Nanokernel Timer
======================================
This code illustrates how an active timer can be stopped prematurely.

.. code-block:: c

   struct nano_timer my_timer;
   uint32_t dummy;

   ...

   /* set timer to expire in 10 ticks */
   nano_timer_init(&my_timer, &dummy);
   nano_fiber_timer_start(&my_timer, 10);

   /* do work while waiting for an input signal to arrive */
   ...

   /* now have input signal, so stop the timer if it is still running */
   nano_fiber_timer_stop(&my_timer);

   /* check to see if the timer expired before it was stopped */
   if (nano_fiber_timer_test(&my_timer) != NULL) {
       printf("Warning: Input signal took too long to arrive!");
   }


APIs
****

The following APIs for a nanokernel timer are provided
by :file:`nanokernel.h.`

+------------------------------------------------+----------------------------+
| Call                                           | Description                |
+================================================+============================+
| :c:func:`nano_timer_init()`                    | Initializes a timer.       |
+------------------------------------------------+----------------------------+
| | :c:func:`nano_task_timer_start()`            | Starts a timer.            |
| | :c:func:`nano_fiber_timer_start()`           |                            |
+------------------------------------------------+----------------------------+
| | :c:func:`nano_task_timer_test()`             | Tests a timer              |
| | :c:func:`nano_fiber_timer_test()`            | to see if it has expired.  |
+------------------------------------------------+----------------------------+
| | :c:func:`nano_task_timer_wait()`             | Waits on a timer           |
| | :c:func:`nano_fiber_timer_wait()`            | until it expires.          |
+------------------------------------------------+----------------------------+
| | :c:func:`nano_task_timer_stop()`             | Forces timer expiration,   |
| | :c:func:`nano_fiber_timer_stop()`            | if not already expired.    |
+------------------------------------------------+----------------------------+

