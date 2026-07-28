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
  expires for the first time.  This is a :c:type:`k_timeout_t` value that
  may be initialized via different units.

* A **period** specifying the time interval between all timer
  expirations after the first one, also a :c:type:`k_timeout_t`. It must be
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

.. note::

   The timer's duration is a **minimum** delay relative to the time the timer
   was started. The timer's period is a **minimum** delay relative to the last
   time the timer "should" have expired. This means that a periodic timer will
   not drift relative to the system timer, and that its periodic delay can be
   shorter or longer than the specified period.

   To ensure a minimum delay until the timer expires, restart the timer from
   within the **expiry function** or after the timer expired.

   The variability of the delays stem from system variables like interrupt
   handling delays and execution time of preceding timer handlers.

When a running timer expires its status is incremented
and the timer executes its expiry function, if one exists;
If a thread is waiting on the timer, it is unblocked.
If the timer's period is zero the timer enters the stopped state;
otherwise, the timer restarts with a new duration equal to the delta between
the time the timer "should" have expired and its period.

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

Timer Observers
***************

When :kconfig:option:`CONFIG_TIMER_OBSERVER` is enabled, code can register
:dfn:`timer observers` that are notified of timer lifecycle events across all
timers in the system. An observer is a set of optional callbacks invoked when a
timer is initialized, started, stopped, or expires. This allows external modules
-- for example tracing, profiling, or power-management code -- to react to timer
activity without modifying kernel internals or the individual timers.

An observer is defined statically with :c:macro:`K_TIMER_OBSERVER_DEFINE`, which
takes pointers to the ``on_init``, ``on_start``, ``on_stop``, and ``on_expiry``
callbacks; any callback that is not needed may be passed as ``NULL``. Because the
expiry callback runs in interrupt context, it must be kept short and
non-blocking.

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

Clock-based Arming
******************

When :kconfig:option:`CONFIG_TIMER_CLOCK` is enabled, a timer may additionally be armed with
:c:func:`k_timer_settime` and queried with :c:func:`k_timer_gettime`, using
:c:struct:`timespec` values measured against a per-timer clock (``SYS_CLOCK_MONOTONIC`` or
``SYS_CLOCK_REALTIME``, bound via :c:macro:`K_TIMER_DEFINE_EXT` or :c:func:`sys_timer_create`).

Clock-based arming differs from :c:func:`k_timer_start` in several deliberate ways:

* **Absolute deadlines are supported** by passing ``SYS_TIMER_ABSTIME``; a deadline already in
  the past expires immediately.
* **"At least" semantics**: a relative duration never expires early. (For historical reasons,
  :c:func:`k_timer_start` fires its *initial* duration up to one tick earlier than an equivalent
  :c:func:`k_sleep`; :c:func:`k_timer_settime` does not inherit that behavior.)
* **Drift-free periodic expiry**: the period advances on an exact tick grid anchored at the
  initial expiry, derived from kernel-maintained absolute scheduled-expiry state. Late or
  coalesced (tickless) wakeups do not accumulate error; if more than one period elapses before
  the timer can be serviced, the timer skips ahead on the same grid rather than replaying
  missed expiries.

.. note::
   Every absolute/relative conversion samples the current time and therefore adds variance;
   when several incoherent conversions stack (absolute to relative to milliseconds to ticks,
   re-sampled every period), the error is amplified and unbounded drift results.
   Clock-based arming performs **at most one conversion per arming operation**, under the timer
   lock, from a single coherent clock snapshot, and always arms with an absolute timeout. This
   is also why :kconfig:option:`CONFIG_TIMER_CLOCK` selects
   :kconfig:option:`CONFIG_TIMEOUT_64BIT` (which gates absolute timeouts): the option is
   dependency-free and fully supported on 32-bit platforms, so absolute timeouts can simply be
   expected everywhere.

Why ``struct timespec`` rather than ``k_timeout_t``
====================================================

:c:type:`k_timeout_t` (and ``k_timepoint_t``) express time in the uptime domain: even their
absolute forms are monotonic tick counts. A ``SYS_CLOCK_REALTIME`` absolute deadline is a
wall-clock time, which only becomes a tick value by sampling the realtime offset - and *where*
that sampling happens is the contract. Converting in the caller would bake the offset in at
conversion time, re-introducing the stacked-conversion variance described above and making
re-targeting on :c:func:`sys_clock_settime` impossible: the kernel would never learn the
wall-clock deadline it is required to preserve. :c:func:`k_timer_settime` therefore accepts
deadlines in the clock's own terms and performs the single conversion kernel-side, under the
timer lock, recording the wall-clock deadline for later re-targeting.

The tick-domain API remains :c:func:`k_timer_start`, which speaks :c:type:`k_timeout_t`
natively (including ``K_TIMEOUT_ABS_TICKS`` for uptime-absolute deadlines); use the
clock-based API when deadlines are measured against a clock.

Function Notification
=====================

A timer created with :c:func:`sys_timer_create` may request **function notification** by
setting :c:member:`k_timer_notify.fn` (requires :kconfig:option:`CONFIG_SYS_THREAD` and
:kconfig:option:`CONFIG_THREAD_DETACH`): each expiry invokes the function, with the
notification value, in a fresh detached thread drawn from the system thread pool with the
requested stack size and priority. A single kernel dispatcher thread (started on first use;
see :kconfig:option:`CONFIG_SYS_TIMER_DISPATCH_STACK_SIZE` and
:kconfig:option:`CONFIG_SYS_TIMER_DISPATCH_PRIO`) accepts the expiries through the reserved
``K_SIG_SYS_TIMER`` signal, so one-pending/overrun semantics are identical to signal
notification. When the creating thread is a user thread, notification threads run in user
mode, in the creator's memory domain, inheriting the creator's object permissions at
dispatch time. This is the substrate for POSIX ``SIGEV_THREAD``.

Realtime Timers and Clock Changes
=================================

With :kconfig:option:`CONFIG_TIMER_REALTIME` enabled, a timer bound to ``SYS_CLOCK_REALTIME``
and armed with an absolute deadline is re-targeted whenever :c:func:`sys_clock_settime` moves
the realtime clock: the timer expires when the *new* wall clock reaches its deadline, as POSIX
requires of ``TIMER_ABSTIME`` ``CLOCK_REALTIME`` timers. Moving the clock forward past a
deadline fires the timer immediately; moving it backward defers the expiry.

Re-targeting applies to the initial expiry only. Once a periodic realtime timer has expired for
the first time, its interval continues on the monotonic tick grid and later clock changes no
longer affect it (matching Linux behavior). A stopped timer is never re-armed by a clock
change.

:c:func:`sys_clock_settime` must not be called from a timer expiry handler.

Signal Delivery and Overrun
***************************

With :kconfig:option:`CONFIG_TIMER_SIGNAL` enabled, a timer may generate a kernel signal on
each expiry, targeted at a specific thread and carrying a user-supplied value with
``si_code == K_SI_TIMER``. Notification is attached at pool-allocation time via
:c:func:`sys_timer_create`. Unlike an expiry function - which runs in interrupt context -
signal delivery happens in the target thread's own context (via a handler installed with
:c:func:`k_sig_action`, or synchronously via :c:func:`k_sig_timedwait`), making it safe for
user-mode consumers. An expiry function and signal generation may coexist on the same timer.

Signal generation follows the POSIX one-pending model: at most one signal instance per timer is
queued at any time. Expirations that occur while an instance is pending are not queued again;
they are accounted as *overruns*, computed from the exact scheduled-expiry grid at the moment
the pending signal is delivered, and readable non-destructively with :c:func:`k_timer_overrun`.
Because the computation is grid-based, expirations coalesced by late or tickless wakeups are
counted correctly, and disarming a timer freezes the count so idle time does not inflate it.

Signal generation is best-effort with respect to signal queue capacity
(:kconfig:option:`CONFIG_SIGNAL_QUEUE_SIZE`): if the queue is full at expiry, that period's
signal is dropped (and retried on the next expiry). Drops are counted per timer and reported
via a rate-limited debug log message, which is compiled out entirely at the default log level.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_TIMER_CLOCK`
* :kconfig:option:`CONFIG_TIMER_REALTIME`
* :kconfig:option:`CONFIG_TIMER_SIGNAL`
* :kconfig:option:`CONFIG_SYS_TIMER`

API Reference
*************

.. doxygengroup:: timer_apis
