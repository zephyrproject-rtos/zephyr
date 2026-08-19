.. _kernel_timing:

Kernel Timing
#############

Zephyr provides a robust and scalable timing framework to enable
reporting and tracking of timed events from hardware timing sources of
arbitrary precision.

Time Units
==========

Kernel time is tracked in several units which are used for different
purposes.

Real time values, typically specified in milliseconds or microseconds,
are the default presentation of time to application code.  They have
the advantages of being universally portable and pervasively
understood, though they may not match the precision of the underlying
hardware perfectly.

The kernel presents a "cycle" count via the :c:func:`k_cycle_get_32`
and :c:func:`k_cycle_get_64` APIs.  The intent is that this counter
represents the fastest cycle counter that the operating system is able
to present to the user (for example, a CPU cycle counter) and that the
read operation is very fast.  The expectation is that very sensitive
application code might use this in a polling manner to achieve maximal
precision.  The frequency of this counter is available from
:c:func:`sys_clock_hw_cycles_per_sec`. On most platforms this is a runtime
constant that evaluates to :kconfig:option:`CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC`
and is fixed for the lifetime of the system. On platforms where the system
timer frequency is not fixed, :c:func:`sys_clock_hw_cycles_per_sec` returns a
runtime value and application code must not assume a single immutable
frequency.

Runtime System Timer Frequency
------------------------------

Some platforms need the system timer frequency to be available at runtime,
either because the timer driver discovers the clock rate from hardware or
because the timer clock rate can change after boot.

Platforms that can change the active system timer frequency at runtime must
enable :kconfig:option:`CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE` and:

* Call :c:func:`z_sys_clock_hw_cycles_per_sec_update` after applying the clock change.
* If the system timer driver caches derived constants (e.g. cycles-per-tick) or
  needs to reprogram hardware when the clock changes, provide a timer-driver
  override of :c:func:`z_sys_clock_hw_cycles_per_sec_update`.

The default implementation of :c:func:`z_sys_clock_hw_cycles_per_sec_update` only updates
the stored frequency value.

.. note::

  :kconfig:option:`CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE` tracks
  the system timer frequency as a single **global** value. It is not compatible
  with per-CPU frequency scaling configurations where different CPUs could
  observe different system timer frequencies.

  When enabled, :c:func:`sys_clock_hw_cycles_per_sec` and time unit conversions
  follow the current runtime value.

For asynchronous timekeeping, the kernel defines a "ticks" concept.  A
"tick" is the internal count in which the kernel does all its internal
uptime and timeout bookkeeping.  Interrupts are expected to be
delivered on tick boundaries to the extent practical, and no
fractional ticks are tracked.  The choice of tick rate is configurable
via :kconfig:option:`CONFIG_SYS_CLOCK_TICKS_PER_SEC`.  Defaults on most
hardware platforms (ones that support setting arbitrary interrupt
timeouts) are expected to be in the range of 10 kHz, with software
emulation platforms and legacy drivers using a more traditional 100 Hz
value.

Conversion
----------

Zephyr provides an extensively enumerated conversion library with
rounding control for all time units.  Any unit of "ms" (milliseconds),
"us" (microseconds), "tick", or "cyc" can be converted to any other.
Control of rounding is provided, and each conversion is available in
"floor" (round down to nearest output unit), "ceil" (round up) and
"near" (round to nearest).  Finally the output precision can be
specified as either 32 or 64 bits.

For example: :c:func:`k_ms_to_ticks_ceil32` will convert a
millisecond input value to the next higher number of ticks, returning
a result truncated to 32 bits of precision; and
:c:func:`k_cyc_to_us_floor64` will convert a measured cycle count
to an elapsed number of microseconds in a full 64 bits of precision.
See the reference documentation for the full enumeration of conversion
routines.

On most platforms, where the various counter rates are integral
multiples of each other and where the output fits within a single
word, these conversions expand to a 2-4 operation sequence, requiring
full precision only where actually required and requested.

.. _kernel_timing_uptime:

Uptime
======

The kernel tracks a system uptime count on behalf of the application.
This is available at all times via :c:func:`k_uptime_get`, which
provides an uptime value in milliseconds since system boot.  This is
expected to be the utility used by most portable application code.

The internal tracking, however, is as a 64 bit integer count of ticks.
Apps with precise timing requirements (that are willing to do their
own conversions to portable real time units) may access this with
:c:func:`k_uptime_ticks`.

:c:func:`k_uptime_delta` can be used to get the time elapsed between a reference
and the current time. The referenced time will be updated to the current uptime,
to easily calculate the next elapsed time.


Timeouts
========

The Zephyr kernel provides many APIs with a "timeout" parameter.
Conceptually, this indicates the time at which an event will occur.
For example:

* Kernel blocking operations like :c:func:`k_sem_take` or
  :c:func:`k_queue_get` may provide a timeout after which the
  routine will return with an error code if no data is available.

* Kernel :c:struct:`k_timer` objects must specify delays for
  their duration and period.

* The kernel :c:struct:`k_work_delayable` API provides a timeout parameter
  indicating when a work queue item will be added to the system queue.

All these values are specified using a :c:type:`k_timeout_t` value.  This is
an opaque struct type that must be initialized using one of a family
of kernel timeout macros.  The most common, :c:macro:`K_MSEC`, defines
a time in milliseconds after the current time.

What is meant by "current time" for relative timeouts depends on the context:

* When scheduling a relative timeout from within a timeout callback (e.g. from
  within the expiry function passed to :c:func:`k_timer_init` or the work handler
  passed to :c:func:`k_work_init_delayable`), "current time" is the exact time at
  which the currently firing timeout was originally scheduled even if the "real
  time" will already have advanced. This is to ensure that timers scheduled from
  within another timer's callback will always be calculated with a precise offset
  to the firing timer. It is thereby possible to fire at regular intervals without
  introducing systematic clock drift over time.

* When scheduling a timeout from application context, "current time" means the
  value returned by :c:func:`k_uptime_ticks` at the time at which the kernel
  receives the timeout value.

Other options for timeout initialization follow the unit conventions
described above: :c:macro:`K_NSEC()`, :c:macro:`K_USEC`, :c:macro:`K_TICKS` and
:c:macro:`K_CYC()` specify timeout values that will expire after specified
numbers of nanoseconds, microseconds, ticks and cycles, respectively.

Precision of :c:type:`k_timeout_t` values is configurable, with the default
being 32 bits.  Large uptime counts in non-tick units will experience
complicated rollover semantics, so it is expected that
timing-sensitive applications with long uptimes will be configured to
use a 64 bit timeout type.

Finally, it is possible to specify timeouts as absolute times since
system boot.  A timeout initialized with :c:macro:`K_TIMEOUT_ABS_MS`
indicates a timeout that will expire after the system uptime reaches
the specified value.  There are likewise nanosecond, microsecond,
cycles and ticks variants of this API.

Timing Internals
================

Timeout Queue
-------------

All Zephyr :c:type:`k_timeout_t` events specified using the API above are
managed in a single, global queue of events.  The action to take on an
event is specified as a callback function pointer provided by the
subsystem requesting the event, along with a :c:struct:`_timeout`
tracking struct that is expected to be embedded within subsystem-defined
data structures (for example: a :c:struct:`wait_q` struct, or a
:c:type:`k_tid_t` thread struct).

Note that all variant units passed via a :c:type:`k_timeout_t` are
converted to ticks once on insertion into the queue.  There are no
multiple-conversion steps internal to the kernel, so precision is
guaranteed at the tick level no matter how many events exist or how long
a timeout might be.

The data structure that holds the queue is selected at build time
through the :kconfig:option:`CONFIG_TIMEOUT_BACKEND` choice.  Only the
front end is shared between backends (the announce path, the SMP
re-entry handling, and the relative versus absolute timeout rules); each
backend supplies the queue itself, so an integrator can match the data
structure to the workload without touching the common code.

The default, :kconfig:option:`CONFIG_TIMEOUT_BACKEND_DLIST`, stores
events in a doubly linked list sorted by expiry, each holding a delta
count in ticks from its predecessor.  Insertion is O(N) in the number of
pending timeouts: inexpensive for the handful a typical system has
pending, but it scales poorly when many are outstanding.  The three
alternative backends, all currently experimental, trade extra memory or
behaviour for faster insertion at scale:

* :kconfig:option:`CONFIG_TIMEOUT_BACKEND_MINHEAP` keeps the events in a
  binary min-heap keyed on absolute expiry, making insertion and removal
  O(log N).  It requires 64-bit ticks
  (:kconfig:option:`CONFIG_TIMEOUT_64BIT`) and a fixed-capacity heap
  (:kconfig:option:`CONFIG_TIMEOUT_HEAP_MAX_ENTRIES`, whose overflow is
  fatal), and it does not preserve the firing order of timeouts that
  expire on the same tick.

* :kconfig:option:`CONFIG_TIMEOUT_BACKEND_WHEEL` is a hierarchical timer
  wheel with O(1) insertion and removal for the near future and a sorted
  overflow list beyond.  It has the largest per-event and static
  footprint, does not preserve same-tick firing order, and wakes a
  tickless-idle CPU periodically because its next-timeout estimate is
  bounded by the wheel period (a power cost the other backends avoid).

* :kconfig:option:`CONFIG_TIMEOUT_BACKEND_BUCKET` is a single-level
  bucketed delta list, a simpler relative of the wheel.  It gives O(1)
  insertion within a tunable near-future window
  (:kconfig:option:`CONFIG_TIMEOUT_BUCKET_LISTS`) and falls back to a
  sorted overflow list beyond it.  It also requires 64-bit ticks, but
  unlike the wheel it preserves same-tick firing order and adds no
  idle-wakeup cost.

The non-default backends target systems that hold many concurrent
timeouts, especially ones clustered in the near future.  For most
applications the delta list remains the appropriate default.

Timer Drivers
-------------

Kernel timing at the tick level is driven by a timer driver.  That interface
and the locking it runs under are described in :ref:`system_timer_drivers`.

Time Slicing
------------

An auxiliary job of the timing subsystem is to provide tick counters
to the scheduler that allow implementation of time slicing of threads.
A thread time-slice cannot be a timeout value, as it does not reflect
a global expiration but instead a per-CPU value that needs to be
tracked independently on each CPU in an SMP context.

Because there may be no other hardware available to drive timeslicing,
Zephyr multiplexes the existing timer driver.  This means that the
value passed to :c:func:`sys_clock_set_timeout` may be clamped to a
smaller value than the current next timeout when a time sliced thread
is currently scheduled.

Subsystems that keep millisecond APIs
-------------------------------------

In general, code like this will port just like applications code will.
Millisecond values from the user may be treated any way the subsystem
likes, and then converted into kernel timeouts using
:c:macro:`K_MSEC()` at the point where they are presented to the
kernel.

Obviously this comes at the cost of not being able to use new
features, like the higher precision timeout constructors or absolute
timeouts.  But for many subsystems with simple needs, this may be
acceptable.

One complexity is :c:macro:`K_FOREVER`.  Subsystems that might have
been able to accept this value to their millisecond API in the past no
longer can, because it is no longer an integral type.  Such code
will need to use a different, integer-valued token to represent
"forever".  :c:macro:`K_NO_WAIT` has the same typesafety concern too,
of course, but as it is (and has always been) simply a numerical zero,
it has a natural porting path.

Subsystems using ``k_timeout_t``
--------------------------------

Ideally, code that takes a "timeout" parameter specifying a time to
wait should be using the kernel native abstraction where possible.
But :c:type:`k_timeout_t` is opaque, and needs to be converted before
it can be inspected by an application.

Some conversions are simple.  Code that needs to test for
:c:macro:`K_FOREVER` can simply use the :c:macro:`K_TIMEOUT_EQ()`
macro to test the opaque struct for equality and take special action.

The more complicated case is when the subsystem needs to take a
timeout and loop, waiting for it to finish while doing some processing
that may require multiple blocking operations on underlying kernel
code.  For example, consider this design:

.. code-block:: c

    void my_wait_for_event(struct my_subsys *obj, int32_t timeout_in_ms)
    {
        while (true) {
            uint32_t start = k_uptime_get_32();

            if (is_event_complete(obj)) {
                return;
            }

            /* Wait for notification of state change */
            k_sem_take(obj->sem, timeout_in_ms);

            /* Subtract elapsed time */
            timeout_in_ms -= (k_uptime_get_32() - start);
        }
    }

This code requires that the timeout value be inspected, which is no
longer possible.  For situations like this, the new API provides the
internal :c:func:`sys_timepoint_calc` and :c:func:`sys_timepoint_timeout` routines
that converts an arbitrary timeout to and from a timepoint value based on
an uptime tick at which it will expire.  So such a loop might look like:


.. code-block:: c

    void my_wait_for_event(struct my_subsys *obj, k_timeout_t timeout)
    {
        /* Compute the end time from the timeout */
        k_timepoint_t end = sys_timepoint_calc(timeout);

        do {
            if (is_event_complete(obj)) {
                return;
            }

            /* Update timeout with remaining time */
            timeout = sys_timepoint_timeout(end);

            /* Wait for notification of state change */
            k_sem_take(obj->sem, timeout);
        } while (!K_TIMEOUT_EQ(timeout, K_NO_WAIT));
    }

Note that :c:func:`sys_timepoint_calc` accepts special values :c:macro:`K_FOREVER`
and :c:macro:`K_NO_WAIT`, and works identically for absolute timeouts as well
as conventional ones. Conversely, :c:func:`sys_timepoint_timeout` may return
:c:macro:`K_FOREVER` or :c:macro:`K_NO_WAIT` if those were used to create
the timepoint, the later also being returned if the timepoint is now in the
past. For simple cases, :c:func:`sys_timepoint_expired` can be used as well.

But some care is still required for subsystems that use those.  Note that
delta timeouts need to be interpreted relative to a "current time",
and obviously that time is the time of the call to
:c:func:`sys_timepoint_calc`.  But the user expects that the time is
the time they passed the timeout to you.  Care must be taken to call
this function just once, as synchronously as possible to the timeout
creation in user code.  It should not be used on a "stored" timeout
value, and should never be called iteratively in a loop.


API Reference
*************

.. doxygengroup:: clock_apis
