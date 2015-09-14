.. _microkernel_timers:

Timer Services
##############

Concepts
********

A microkernel timer allows a task to determine whether or not a specified
time limit has been reached while the task is busy performing other work.
The timer uses the kernel's system clock to monitor the passage of time,
as measured in ticks.

Any number of microkernel timers can be defined in a microkernel system.
Each timer has a unique identifier, which allows it to be distinguished
from other timers.

A task that wants to use a timer must first allocate an unused timer
from the set of microkernel timers. A task can allocate more than one timer
when it needs to monitor multiple time intervals simultaneously.

A timer is started by specifying a duration, a period, and an associated
microkernel semaphore identifier. Duration is the number of ticks
the timer counts before it expires for the first time, and period is the
number of ticks the timer counts before it expires each time thereafter.
Each time the timer expires the timer gives the semaphore.
The semaphore's state can be examined by the task at any time
to allow the task to determine if a given time limit has been reached or not.

When the timer's period is set to zero the timer stops automatically
once the duration is reached and the semaphore is given. When the period
is non-zero the timer restarts automatically using a new duration equal
to its period; when this new duration has elapsed the timer gives the
semaphore again and restarts. For example, a timer can be set to expire
after 5 ticks, and then re-expire every 20 ticks thereafter,
resulting in the semaphore being given 3 times after 50 ticks have elapsed.

.. note::
   Care must be taken when specifying the duration of a microkernel timer,
   since the first tick measured by the timer after it is started will be
   less than a full tick interval. For example, when the system clock period
   is 10 milliseconds starting a timer than expires after 1 tick will result
   in the semaphore being given anywhere from a fraction of a millisecond
   later to just slightly less than 10 milliseconds later. To ensure that
   a timer doesn't expire for at least N ticks it is necessary to specify
   a duration of N+1 ticks. This adjustment is not required when specifying
   the period of a timer, which always corresponds to full tick intervals.

A running microkernel timer can be cancelled or restarted by a task prior
to its expiration. Cancelling a timer that has already expired does not
affect the state of the associated semaphore. Likewise, restarting a
timer that has already expired is equivalent to stopping the timer and
then starting it afresh.

When a task no longer needs a timer it should free the timer.
This makes the timer available for reallocation.


Purpose
*******

Use a microkernel timer to determine whether or not a specified number
of system clock ticks have elapsed while the task is busy performing
other work.

.. note::
   If a task has no other work to perform while waiting for time to pass
   it can simply call :c:func:`task_sleep()`.

.. note::
   The microkernel provides additional APIs that allow a task to monitor
   the system clock, as well as the higher precision hardware clock,
   without using a microkernel timer.


Usage
*****

Configuring Microkernel Timers
==============================

Set the :option:`NUM_TIMER_PACKETS` configuration option
to specify the number of timer-related command packets available
in the application. This value should be equal to or greater than
the sum of the following quantities:

* The number of microkernel timers.
* The number of tasks.

.. note::
   Unlike most other microkernel object types, microkernel timers are defined
   as a group using a configuration option, rather than as individual
   public objects in an .MDEF file or private objects in a source file.


Example: Allocating a Microkernel Timer
=======================================

This code allocates an unused timer.

.. code-block:: c

   ktimer_t timer_id;

   timer_id = task_timer_alloc();


Example: Starting a One Shot Microkernel Timer
==============================================
This code uses a timer to limit the amount of time a task
spends gathering data by monitoring the status of a microkernel semaphore
that is set when the timer expires. Since the timer is started with
a period of zero, it stops automatically once it expires.

.. code-block:: c

   ktimer_t timer_id;
   ksem_t my_sem;

   ...

   /* set timer to expire in 10 ticks */
   task_timer_start(timer_id, 10, 0, my_sem);

   /* gather data until timer expires */
   do {
       ...
   } while (task_sem_take(my_sem) != RC_OK);

   /* process the new data */
   ...


Example: Starting a Periodic Microkernel Timer
==============================================
This code is similar to the previous example, except that the timer
automatically restarts every time it expires. This approach eliminates
the overhead of having the task explicitly issue a request to
reactivate the timer.

.. code-block:: c

   ktimer_t timer_id;
   ksem_t my_sem;

   ...

   /* set timer to expire every 10 ticks */
   task_timer_start(timer_id, 10, 10, my_sem);

   while (1) {
       /* gather data until timer expires */
       do {
           ...
       } while (task_sem_take(my_sem) != RC_OK);

       /* process the new data, then loop around to get more */
       ...
   }


Example: Cancelling a Microkernel Timer
=======================================
This code illustrates how an active timer can be stopped prematurely.

.. code-block:: c

   ktimer_t timer_id;
   ksem_t my_sem;

   ...

   /* set timer to expire in 10 ticks */
   task_timer_start(timer_id, 10, 0, my_sem);

   /* do work while waiting for input to arrive */
   ...

   /* now have input, so stop the timer if it is still running */
   task_timer_stop(timer_id);

   /* check to see if the timer expired before it was stopped */
   if (task_sem_take(my_sem) == RC_OK) {
       printf("Warning: Input took too long to arrive!");
   }


Example: Freeing a Microkernel Timer
====================================
This code allows a task to relinquish a previously allocated timer
so it can be used by other tasks.

.. code-block:: c

   task_timer_free(timer_id);



APIs
****

The following microkernel timer APIs are provided by :file:`microkernel.h`:

+----------------------------------------+-----------------------------------+
| Call                                   | Description                       |
+========================================+===================================+
| :cpp:func:`task_timer_alloc()`         | Allocates an unused timer.        |
+----------------------------------------+-----------------------------------+
| :cpp:func:`task_timer_start()`         | Starts a timer.                   |
+----------------------------------------+-----------------------------------+
| :cpp:func:`task_timer_restart()`       | Restarts a timer.                 |
+----------------------------------------+-----------------------------------+
| :cpp:func:`task_timer_stop()`          | Cancels a timer.                  |
+----------------------------------------+-----------------------------------+
| :cpp:func:`task_timer_free()`          | Marks timer as unused.            |
+----------------------------------------+-----------------------------------+

