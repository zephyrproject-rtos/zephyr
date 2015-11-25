.. _kernel_clocks:

Kernel Clocks
#############

Concepts
********

The kernel supports two distinct clocks.

* A 64-bit *system clock*, which is the foundation for the kernel's
  time-based services. This clock is a counter measured in *ticks*,
  and increments at a frequency determined by the application.

  The kernel allows this clock to be accessed directly by reading
  the timer. It can also be accessed indirectly by using a kernel
  timer or timeout capability.

* A 32-bit *hardware clock*, which is used as the source of the ticks
  for the system clock. This clock is a counter measured in unspecified
  units (called *cycles*), and increments at a frequency
  determined by the platform hardware.

  The kernel allows this clock to be accessed directly by reading
  the timer.

The kernel also provides a number of variables that can be used
to convert the time units used by the clocks into standard time units
(e.g. seconds, milliseconds, nanoseconds, etc), and to convert between
the two types of clock time units.

Purpose
*******

Use the system clock for time-based processing that does not require
high precision, such as implementing time limits or time delays.

Use the hardware clock for time-based processing that requires higher
precision than the system clock can provide, such as fine-grained
time measurements.

.. note::
   The high frequency of the hardware clock, combined with its 32-bit size,
   means that counter rollover must be taken into account when taking
   high-precision measurements over an extended period of time.

Usage
*****

Configuring the Kernel Clocks
=============================

Use the :option:`SYS_CLOCK_TICKS_PER_SEC` configuration option
to specify how many ticks occur every second. Setting this value
to zero disables all system clock and hardware clock capabilities.

.. note::
   Making the system clock frequency value larger allows the system clock
   to provide finer-grained timing, but also increases the amount of work
   the kernel has to do to process ticks (since they occur more frequently).

Example: Measuring Time with Normal Precision
=============================================
This code uses the system clock to determine how many ticks have elapsed
between two points in time.

.. code-block:: c

   int64_t time_stamp;
   int64_t ticks_spent;

   /* capture initial time stamp */
   time_stamp = sys_tick_get();

   /* do work for some (extended) period of time */
   ...

   /* compute how long the work took & update time stamp */
   ticks_spent = sys_tick_delta(&time_stamp);

Example: Measuring Time with High Precision
===========================================
This code uses the hardware clock to determine how many ticks have elapsed
between two points in time.

.. code-block:: c

   uint32_t start_time;
   uint32_t stop_time;
   uint32_t cycles_spent;
   uint32_t nanoseconds_spent;

   /* capture initial time stamp */
   start_time = sys_cycle_get_32();

   /* do work for some (short) period of time */
   ...

   /* capture final time stamp */
   stop_time = sys_cycle_get_32();

   /* compute how long the work took (assumes no counter rollover) */
   cycles_spent = stop_time - start_time;
   nanoseconds_spent = SYS_CLOCK_HW_CYCLES_TO_NS(cycles_spent);


APIs
****

The following kernel clock APIs are provided by :file:`microkernel.h`:

:cpp:func:`sys_tick_get()`, :cpp:func:`sys_tick_get_32()`
   Read the system clock.

:cpp:func:`sys_tick_delta()`, :cpp:func:`sys_tick_delta_32()`
   Compute the elapsed time since an earlier system clock reading.

:cpp:func:`task_cycle_get_32()`, :c:func:`fiber_cycle_get_32()`,
:c:func:`isr_cycle_get_32()`
   Read the hardware clock.

The following kernel clock APIs are provided by :file:`microkernel.h`
and by :file:`nanokernel.h`:

:cpp:func:`sys_tick_get()`, :cpp:func:`sys_tick_get_32()`
   Read the system clock.

:cpp:func:`sys_tick_delta()`, :cpp:func:`sys_tick_delta_32()`
   Compute the elapsed time since an earlier system clock reading.

:cpp:func:`nano_cycle_get_32()`, :cpp:func:`sys_cycle_get_32()`
   Reads hardware clock.

The following kernel clock variables are provided by :file:`microkernel.h`
and by :file:`nanokernel.h`:

:c:data:`sys_clock_ticks_per_sec`
   The number of system clock ticks in a single second.

:c:data:`sys_clock_hw_cycles_per_sec`
   The number of hardware clock cycles in a single second.

:c:data:`sys_clock_us_per_tick`
   The number of microseconds in a single system clock tick.

:c:data:`sys_clock_hw_cycles_per_tick`
   The number of hardware clock cycles in a single system clock tick.
