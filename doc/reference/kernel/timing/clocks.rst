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

The kernel presents a "cycle" count via the
:c:func:`k_cycle_get_32` API.  The intent is that this counter
represents the fastest cycle counter that the operating system is able
to present to the user (for example, a CPU cycle counter) and that the
read operation is very fast.  The expectation is that very sensitive
application code might use this in a polling manner to achieve maximal
precision.  The frequency of this counter is required to be steady
over time, and is available from
:c:func:`sys_clock_hw_cycles_per_sec` (which on almost all
platforms is a runtime constant that evaluates to
CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC).

For asynchronous timekeeping, the kernel defines a "ticks" concept.  A
"tick" is the internal count in which the kernel does all its internal
uptime and timeout bookeeping.  Interrupts are expected to be
delivered on tick boundaries to the extent practical, and no
fractional ticks are tracked.  The choice of tick rate is configurable
via :c:option:`CONFIG_SYS_CLOCK_TICKS_PER_SEC`.  Defaults on most
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

* The kernel ``k_delayed_work`` API provides a timeout parameter
  indicating when a work queue item will be added to the system queue.

All these values are specified using a ``k_timeout_t`` value.  This is
an opaque struct type that must be initialized using one of a family
of kernel timeout macros.  The most common, ``K_MSEC()``, defines
a time in milliseconds after the current time (strictly: the time at
which the kernel receives the timeout value).

Other options for timeout initialization follow the unit conventions
described above: ``K_NSEC()``, ``K_USEC()``, ``K_TICKS()`` and
``K_CYC()`` specify timeout values that will expire after specified
numbers of nanoseconds, microseconds, ticks and cycles, respectively.

Precision of ``k_timeout_t`` values is configurable, with the default
being 32 bits.  Large uptime counts in non-tick units will experience
complicated rollover semantics, so it is expected that
timing-sensitive applications with long uptimes will be configured to
use a 64 bit timeout type.

Finally, it is possible to specify timeouts as absolute times since
system boot.  A timeout initialized with ``K_TIMEOUT_ABS_MS()``
indicates a timeout that will expire after the system uptime reaches
the specified value.  There are likewise nanosecond, microsecond,
cycles and ticks variants of this API.

Timing Internals
================

Timeout Queue
-------------

All Zephyr ``k_timeout_t`` events specified using the API above are
managed in a single, global queue of events.  Each event is stored in
a double-linked list, with an attendant delta count in ticks from the
previous event.  The action to take on an event is specified as a
callback function pointer provided by the subsystem requesting the
event, along with a :c:struct:`_timeout` tracking struct that is
expected to be embedded within subsystem-defined data structures (for
example: a ``struct wait_q``, or a ``k_tid_t`` thread struct).

Note that all variant units passed via a ``k_timeout_t`` are converted
to ticks once on insertion into the list.  There no
multiple-conversion steps internal to the kernel, so precision is
guaranteed at the tick level no matter how many events exist or how
long a timeout might be.

Note that the list structure means that the CPU work involved in
managing large numbers of timeouts is quadratic in the number of
active timeouts.  The API design of the timeout queue was intended to
permit a more scalable backend data structure, but no such
implementation exists currently.

Timer Drivers
-------------

Kernel timing at the tick level is driven by a timer driver with a
comparatively simple API.

* The driver is expected to be able to "announce" new ticks to the
  kernel via the ``z_clock_announce()`` call, which passes an integer
  number of ticks that have elapsed since the last announce call (or
  system boot).  These calls can occur at any time, but the driver is
  expected to attempt to ensure (to the extent practical given
  interrupt latency interactions) that they occur near tick boundaries
  (i.e. not "halfway through" a tick), and most importantly that they
  be correct over time and subject to minimal skew vs. other counters
  and real world time.

* The driver is expected to provide a ``z_clock_set_timeout()`` call
  to the kernel which indicates how many ticks may elapse before the
  kernel must receive an announce call to trigger registered timeouts.
  It is legal to announce new ticks before that moment (though they
  must be correct) but delay after that will cause events to be
  missed.  Note that the timeout value passed here is in a delta from
  current time, but that does not absolve the driver of the
  requirement to provide ticks at a steady rate over time.  Naive
  implementations of this function are subject to bugs where the
  fractional tick gets "reset" incorrectly and causes clock skew.

* The driver is expected to provide a ``z_clock_elapsed()`` call which
  provides a current indication of how many ticks have elapsed (as
  compared to a real world clock) since the last call to
  ``z_clock_announce()``, which the kernel needs to test newly
  arriving timeouts for expiration.

Note that a natural implementation of this API results in a "tickless"
kernel, which receives and processes timer interrupts only for
registered events, relying on programmable hardware counters to
provide irregular interrupts.  But a traditional, "ticked" or "dumb"
counter driver can be trivially implemented also:

* The driver can receive interrupts at a regular rate corresponding to
  the OS tick rate, calling z_clock_anounce() with an argument of one
  each time.

* The driver can ignore calls to ``z_clock_set_timeout()``, as every
  tick will be announced regardless of timeout status.

* The driver can return zero for every call to ``z_clock_elapsed()``
  as no more than one tick can be detected as having elapsed (because
  otherwise an interrupt would have been received).

SMP Details
-----------

In general, the timer API described above does not change when run in
a multiprocessor context.  The kernel will internally synchronize all
access appropriately, and ensure that all critical sections are small
and minimal.  But some notes are important to detail:

* Zephyr is agnostic about which CPU services timer interrupts.  It is
  not illegal (though probably undesirable in some circumstances) to
  have every timer interrupt handled on a single processor.  Existing
  SMP architectures implement symmetric timer drivers.

* The ``z_clock_announce()`` call is expected to be globally
  synchronized at the driver level.  The kernel does not do any
  per-CPU tracking, and expects that if two timer interrupts fire near
  simultaneously, that only one will provide the current tick count to
  the timing subsystem.  The other may legally provide a tick count of
  zero if no ticks have elapsed.  It should not "skip" the announce
  call because of timeslicing requirements (see below).

* Some SMP hardware uses a single, global timer device, others use a
  per-CPU counter.  The complexity here (for example: ensuring counter
  synchronization between CPUs) is expected to be managed by the
  driver, not the kernel.

* The next timeout value passed back to the driver via
  :c:func:`z_clock_set_timeout` is done identically for every CPU.
  So by default, every CPU will see simultaneous timer interrupts for
  every event, even though by definition only one of them should see a
  non-zero ticks argument to ``z_clock_announce()``.  This is probably
  a correct default for timing sensitive applications (because it
  minimizes the chance that an errant ISR or interrupt lock will delay
  a timeout), but may be a performance problem in some cases.  The
  current design expects that any such optimization is the
  responsibility of the timer driver.

Time Slicing
------------

An auxiliary job of the timing subsystem is to provide tick counters
to the scheduler that allow implementation of time slicing of threads.
A thread time-slice cannot be a timeout value, as it does not reflect
a global expiration but instead a per-CPU value that needs to be
tracked independently on each CPU in an SMP context.

Because there may be no other hardware available to drive timeslicing,
Zephyr multiplexes the existing timer driver.  This means that the
value passed to :c:func:`z_clock_set_timeout` may be clamped to a
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
longer can, becauase it is no longer an intergral type.  Such code
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
longer possible.  For situations like this, the new API provides an
internal :c:func:`z_timeout_end_calc` routine that converts an
arbitrary timeout to the uptime value in ticks at which it will
expire.  So such a loop might look like:


.. code-block:: c

    void my_wait_for_event(struct my_subsys *obj, k_timeout_t timeout_in_ms)
    {
        /* Compute the end time from the timeout */
        uint64_t end = z_timeout_end_calc(timeout_in_ms);

        while (end > k_uptime_ticks()) {
            if (is_event_complete(obj)) {
                return;
            }

            /* Wait for notification of state change */
            k_sem_take(obj->sem, timeout_in_ms);
        }
    }

Note that :c:func:`z_timeout_end_calc` returns values in units of
ticks, to prevent conversion aliasing, is always presented at 64 bit
uptime precision to prevent rollover bugs, handles special
:c:macro:`K_FOREVER` naturally (as ``UINT64_MAX``), and works
identically for absolute timeouts as well as conventional ones.

But some care is still required for subsystems that use it.  Note that
delta timeouts need to be interpreted relative to a "current time",
and obviously that time is the time of the call to
:c:func:`z_timeout_end_calc`.  But the user expects that the time is
the time they passed the timeout to you.  Care must be taken to call
this function just once, as synchronously as possible to the timeout
creation in user code.  It should not be used on a "stored" timeout
value, and should never be called iteratively in a loop.
