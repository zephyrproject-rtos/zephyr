.. _nanokernel_timers:

Timer Services
##############

Concepts
********

The nanokernel's timer object type uses the kernel's system clock to monitor
the passage of time, as measured in ticks. It is mainly intended for use
by fibers.

A nanokernel timer allows a fiber or task to determine if a specified time
limit has been reached while the thread itself is busy performing other work.
A thread can use more than one timer when it needs to monitor multiple time
intervals simultaneously.

A nanokernel timer points to a *user data structure* that is supplied by the
thread that uses it; this pointer is returned when the timer expires.
The user data structure must be at least 4 bytes long and aligned on a 4-byte
boundary, as the kernel reserves the first 32 bits of this area for its own use.
Any remaining bytes of this area can be used to hold data that is helpful
to the thread that uses the timer.

Any number of nanokernel timers can be defined. Each timer is a distinct
variable of type :cpp:type:`struct nano_timer`, and is referenced using a pointer
to that variable. A timer must be initialized with its user data structure
before it can be used.

A nanokernel timer is started by specifying a duration, which is the number
of ticks the timer counts before it expires.

.. note::
   Care must be taken when specifying the duration of a nanokernel timer,
   since the first tick measured by the timer after it is started will be
   less than a full tick interval. For example, when the system clock period
   is 10 milliseconds starting a timer than expires after 1 tick will result
   in the timer expiring anywhere from a fraction of a millisecond
   later to just slightly less than 10 milliseconds later. To ensure that
   a timer doesn't expire for at least N ticks it is necessary to specify
   a duration of N+1 ticks.

Once started, a nanokernel timer can be tested in either a non-blocking or
blocking manner to allow a thread to determine if the timer has expired.
If the timer has expired the kernel returns the pointer to the user data
structure. If the timer has not expired the kernel either returns
:c:macro:`NULL` (for a non-blocking test) or waits for the timer to expire
(for a blocking test).

.. note::
   The nanokernel does not allow more than one thread to wait on a nanokernel
   timer at any given time. If a second thread starts waiting only the first
   waiting thread wakes up when the timer expires and the second thread
   continues waiting.

   A task that waits on a nanokernel timer does a busy wait. This is
   not an issue for a nanokernel application's background task; however, in
   a microkernel application a task that waits on a nanokernel timer remains
   the current task, which prevents other tasks of equal or lower priority
   from doing useful work.

A nanokernel timer can be cancelled after it has been started. Cancelling
a timer while it is still running causes the timer to expire immediately,
thereby unblocking any thread waiting on the timer. Cancelling a timer
that has already expired has no effect on the timer.

A nanokernel timer can be reused once it has expired, but must **not** be
restarted while it is still running. If desired, a timer can be re-initialized
with a different user data structure before it is started again.


Purpose
*******

Use a nanokernel timer to determine whether or not a specified number
of system clock ticks have elapsed while a fiber or task is busy performing
other work.

.. note::
   If a fiber or task has no other work to perform while waiting
   for time to pass it can simply call :cpp:func:`fiber_sleep()`
   or :cpp:func:`task_sleep()`, respectively.

.. note::
   The kernel provides additional APIs that allow a fiber or task to monitor
   the system clock, as well as the higher precision hardware clock,
   without using a nanokernel timer.


Usage
*****

Example: Initializing a Nanokernel Timer
========================================

This code initializes a nanokernel timer.

.. code-block:: c

   struct nano_timer my_timer;
   uint32_t data_area[3] = { 0, 1111, 2222 };

   nano_timer_init(&my_timer, data_area);


Example: Starting a Nanokernel Timer
====================================
This code uses the above nanokernel timer to limit the amount of time a fiber
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
This code illustrates how an active nanokernel timer can be stopped prematurely.

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
by :file:`nanokernel.h`:

:cpp:func:`nano_timer_init()`
   Initializes a timer.

:cpp:func:`nano_task_timer_start()`, :cpp:func:`nano_fiber_timer_start()`,
:cpp:func:`nano_isr_timer_start()`, :cpp:func:`nano_timer_start()`
   Start a timer.

:cpp:func:`nano_task_timer_test()`, :cpp:func:`nano_fiber_timer_test()`,
:cpp:func:`nano_isr_timer_test()`, :cpp:func:`nano_timer_test()`
   Test a timer to see if it has expired.

:cpp:func:`nano_task_timer_wait()`, :cpp:func:`nano_fiber_timer_wait()`,
:cpp:func:`nano_timer_wait()`
   Wait on a timer until it expires.

:cpp:func:`nano_task_timer_stop()`, :cpp:func:`nano_fiber_timer_stop()`,
:cpp:func:`nano_isr_timer_stop()`, :cpp:func:`nano_timer_stop()`
   Force timer expiration, if not already expired.
