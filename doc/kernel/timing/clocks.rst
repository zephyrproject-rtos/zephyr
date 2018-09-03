.. _clocks_v2:

Kernel Clocks
#############

The kernel's clocks are the foundation for all of its time-based services.

.. contents::
    :local:
    :depth: 2

Concepts
********

The kernel supports two distinct clocks.

* The 32-bit **hardware clock** is a high precision counter that tracks time
  in unspecified units called **cycles**. The duration of a cycle is determined
  by the board hardware used by the kernel, and is typically measured
  in nanoseconds.

* The 64-bit **system clock** is a counter that tracks the number of
  **ticks** that have elapsed since the kernel was initialized. The duration
  of a tick is configurable, and typically ranges from 1 millisecond to
  100 milliseconds.

The kernel also provides a number of variables that can be used
to convert the time units used by the clocks into standard time units
(e.g. seconds, milliseconds, nanoseconds, etc), and to convert between
the two types of clock time units.

The system clock is used by most of the kernel's time-based services, including
kernel timer objects and the timeouts supported by other kernel object types.
For convenience, the kernel's APIs allow time durations to be specified
in milliseconds, and automatically converts them to the corresponding
number of ticks.

The hardware clock can be used to measure time with higher precision than
that provided by kernel services based on the system clock.

.. _clock_limitations:

Clock Limitations
=================

The system clock's tick count is derived from the hardware clock's cycle
count. The kernel determines how many clock cycles correspond to the desired
tick frequency, then programs the hardware clock to generate an interrupt
after that many cycles; each interrupt corresponds to a single tick.

.. note::
    Configuring a smaller tick duration permits finer-grained timing,
    but also increases the amount of work the kernel has to do to process
    tick interrupts since they occur more frequently. Setting the tick
    duration to zero disables *both* kernel clocks, as well as their
    associated services.

Any millisecond-based time interval specified using a kernel API
represents the **minimum** delay that will occur,
and may actually take longer than the amount of time requested.

For example, specifying a timeout delay of 100 ms when attempting to take
a semaphore means that the kernel will never terminate the operation
and report failure before at least 100 ms have elapsed. However,
it is possible that the operation may take longer than 100 ms to complete,
and may either complete successfully during the additional time
or fail at the end of the added time.

The amount of added time that occurs during a kernel object operation
depends on the following factors.

* The added time introduced by rounding up the specified time interval
  when converting from milliseconds to ticks. For example, if a tick duration
  of 10 ms is being used, a specified delay of 25 ms will be rounded up
  to 30 ms.

* The added time introduced by having to wait for the next tick interrupt
  before a delay can be properly tracked. For example, if a tick duration
  of 10 ms is being used, a specified delay of 20 ms requires the kernel
  to wait for 3 ticks to occur (rather than only 2), since the first tick
  can occur at any time from the next fraction of a millisecond to just
  slightly less than 10 ms; only after the first tick has occurred does
  the kernel know the next 2 ticks will take 20 ms.

Implementation
**************

Measuring Time with Normal Precision
====================================

This code uses the system clock to determine how much time has elapsed
between two points in time.

.. code-block:: c

    s64_t time_stamp;
    s64_t milliseconds_spent;

    /* capture initial time stamp */
    time_stamp = k_uptime_get();

    /* do work for some (extended) period of time */
    ...

    /* compute how long the work took (also updates the time stamp) */
    milliseconds_spent = k_uptime_delta(&time_stamp);

Measuring Time with High Precision
==================================

This code uses the hardware clock to determine how much time has elapsed
between two points in time.

.. code-block:: c

    u32_t start_time;
    u32_t stop_time;
    u32_t cycles_spent;
    u32_t nanoseconds_spent;

    /* capture initial time stamp */
    start_time = k_cycle_get_32();

    /* do work for some (short) period of time */
    ...

    /* capture final time stamp */
    stop_time = k_cycle_get_32();

    /* compute how long the work took (assumes no counter rollover) */
    cycles_spent = stop_time - start_time;
    nanoseconds_spent = SYS_CLOCK_HW_CYCLES_TO_NS(cycles_spent);

Suggested Uses
**************

Use services based on the system clock for time-based processing
that does not require high precision,
such as :ref:`timer objects <timers_v2>` or :ref:`thread_sleeping`.

Use services based on the hardware clock for time-based processing
that requires higher precision than the system clock can provide,
such as :ref:`busy_waiting` or fine-grained time measurements.

.. note::
    The high frequency of the hardware clock, combined with its 32-bit size,
    means that counter rollover must be taken into account when taking
    high-precision measurements over an extended period of time.

Configuration
*************

Related configuration options:

* :option:`CONFIG_SYS_CLOCK_TICKS_PER_SEC`

APIs
****

The following kernel clock APIs are provided by :file:`kernel.h`:

* :cpp:func:`k_uptime_get()`
* :cpp:func:`k_uptime_get_32()`
* :cpp:func:`k_uptime_delta()`
* :cpp:func:`k_uptime_delta_32()`
* :cpp:func:`k_cycle_get_32()`
* :c:macro:`SYS_CLOCK_HW_CYCLES_TO_NS`
* :c:macro:`K_NO_WAIT`
* :c:macro:`K_MSEC`
* :c:macro:`K_SECONDS`
* :c:macro:`K_MINUTES`
* :c:macro:`K_HOURS`
* :c:macro:`K_FOREVER`
