.. _timers_v2:

Timers
######

A :dfn:`timer` is a kernel object that measures the passage of time
using the kernel's system clock. When a timer's specified time limit
is reached it can perform an application-defined action,
or it can simply record the expiration and wait for the application
to read its status.

.. contents::
    :local:
    :depth: 2

Concepts
********

Any number of timers can be defined (limited only by available RAM). Each timer
is referenced by its memory address.

A timer has the following key properties:

* A **duration** specifying the time interval before the timer
  expires for the first time.  This is a ``k_timeout_t`` value that
  may be initialized via different units.

* A **period** specifying the time interval between all timer
  expirations after the first one, also a ``k_timeout_t``. It must be
  non-negative.  A period of ``K_NO_WAIT`` (i.e. zero) or
  ``K_FOREVER`` means that the timer is a one-shot timer that stops
  after a single expiration. (For example then, if a timer is started
  with a duration of 200 and a period of 75, it will first expire
  after 200 ms and then every 75 ms after that.)

* An **expiry function** that is executed each time the timer expires.
  The function is executed by the system clock interrupt handler.
  If no expiry function is required a ``NULL`` function can be specified.

* A **stop function** that is executed if the timer is stopped prematurely
  while running. The function is executed by the thread that stops the timer.
  If no stop function is required a ``NULL`` function can be specified.

* A **status** value that indicates how many times the timer has expired
  since the status value was last read.

A timer must be initialized before it can be used. This specifies its
expiry function and stop function values, sets the timer's status to zero,
and puts the timer into the **stopped** state.

A timer is **started** by specifying a duration and a period.
The timer's status is reset to zero, and then the timer enters
the **running** state and begins counting down towards expiry.

Note that the timer's duration and period parameters specify
**minimum** delays that will elapse.  Because of internal system timer
precision (and potentially runtime interactions like interrupt delay)
it is possible that more time may have passed as measured by reads
from the relevant system time APIs.  But at least this much time is
guaranteed to have elapsed.

When a running timer expires its status is incremented
and the timer executes its expiry function, if one exists;
If a thread is waiting on the timer, it is unblocked.
If the timer's period is zero the timer enters the stopped state;
otherwise, the timer restarts with a new duration equal to its period.

A running timer can be stopped in mid-countdown, if desired.
The timer's status is left unchanged, then the timer enters the stopped state
and executes its stop function, if one exists.
If a thread is waiting on the timer, it is unblocked.
Attempting to stop a non-running timer is permitted,
but has no effect on the timer since it is already stopped.

A running timer can be restarted in mid-countdown, if desired.
The timer's status is reset to zero, then the timer begins counting down
using the new duration and period values specified by the caller.
If a thread is waiting on the timer, it continues waiting.

A timer's status can be read directly at any time to determine how many times
the timer has expired since its status was last read.
Reading a timer's status resets its value to zero.
The amount of time remaining before the timer expires can also be read;
a value of zero indicates that the timer is stopped.

A thread may read a timer's status indirectly by **synchronizing**
with the timer. This blocks the thread until the timer's status is non-zero
(indicating that it has expired at least once) or the timer is stopped;
if the timer status is already non-zero or the timer is already stopped
the thread continues without waiting. The synchronization operation
returns the timer's status and resets it to zero.

.. note::
    Only a single user should examine the status of any given timer,
    since reading the status (directly or indirectly) changes its value.
    Similarly, only a single thread at a time should synchronize
    with a given timer. ISRs are not permitted to synchronize with timers,
    since ISRs are not allowed to block.

Implementation
**************

Defining a Timer
================

A timer is defined using a variable of type :c:struct:`k_timer`.
It must then be initialized by calling :c:func:`k_timer_init`.

The following code defines and initializes a timer.

.. code-block:: c

    struct k_timer my_timer;
    extern void my_expiry_function(struct k_timer *timer_id);

    k_timer_init(&my_timer, my_expiry_function, NULL);

Alternatively, a timer can be defined and initialized at compile time
by calling :c:macro:`K_TIMER_DEFINE`.

The following code has the same effect as the code segment above.

.. code-block:: c

    K_TIMER_DEFINE(my_timer, my_expiry_function, NULL);

Using a Timer Expiry Function
=============================

The following code uses a timer to perform a non-trivial action on a periodic
basis. Since the required work cannot be done at the interrupt level,
the timer's expiry function submits a work item to the
:ref:`system workqueue <workqueues_v2>`, whose thread performs the work.

.. code-block:: c

    void my_work_handler(struct k_work *work)
    {
        /* do the processing that needs to be done periodically */
        ...
    }

    K_WORK_DEFINE(my_work, my_work_handler);

    void my_timer_handler(struct k_timer *dummy)
    {
        k_work_submit(&my_work);
    }

    K_TIMER_DEFINE(my_timer, my_timer_handler, NULL);

    ...

    /* start a periodic timer that expires once every second */
    k_timer_start(&my_timer, K_SECONDS(1), K_SECONDS(1));

Reading Timer Status
====================

The following code reads a timer's status directly to determine
if the timer has expired or not.

.. code-block:: c

    K_TIMER_DEFINE(my_status_timer, NULL, NULL);

    ...

    /* start a one-shot timer that expires after 200 ms */
    k_timer_start(&my_status_timer, K_MSEC(200), K_NO_WAIT);

    /* do work */
    ...

    /* check timer status */
    if (k_timer_status_get(&my_status_timer) > 0) {
        /* timer has expired */
    } else if (k_timer_remaining_get(&my_status_timer) == 0) {
        /* timer was stopped (by someone else) before expiring */
    } else {
        /* timer is still running */
    }

Using Timer Status Synchronization
==================================

The following code performs timer status synchronization to allow a thread
to do useful work while ensuring that a pair of protocol operations
are separated by the specified time interval.

.. code-block:: c

    K_TIMER_DEFINE(my_sync_timer, NULL, NULL);

    ...

    /* do first protocol operation */
    ...

    /* start a one-shot timer that expires after 500 ms */
    k_timer_start(&my_sync_timer, K_MSEC(500), K_NO_WAIT);

    /* do other work */
    ...

    /* ensure timer has expired (waiting for expiry, if necessary) */
    k_timer_status_sync(&my_sync_timer);

    /* do second protocol operation */
    ...

.. note::
    If the thread had no other work to do it could simply sleep
    between the two protocol operations, without using a timer.

Suggested Uses
**************

Use a timer to initiate an asynchronous operation after a specified
amount of time.

Use a timer to determine whether or not a specified amount of time has
elapsed.  In particular, timers should be used when higher precision
and/or unit control is required than that afforded by the simpler
:c:func:`k_sleep` and :c:func:`k_usleep` calls.

Use a timer to perform other work while carrying out operations
involving time limits.

.. note::
   If a thread needs to measure the time required to perform an operation
   it can read the :ref:`system clock or the hardware clock <kernel_timing>`
   directly, rather than using a timer.

Configuration Options
*********************

Related configuration options:

* None

API Reference
*************

.. doxygengroup:: timer_apis
