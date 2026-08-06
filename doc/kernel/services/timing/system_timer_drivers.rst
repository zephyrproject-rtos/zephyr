.. _system_timer_drivers:

System Timer Drivers
####################

This page describes the interface between the kernel's tick accounting and
the driver for the hardware that produces those ticks.  The kernel side of
that accounting, and everything an application sees, is described in
:ref:`kernel_timing`.

Timer Drivers
=============

Kernel timing at the tick level is driven by a timer driver with a
comparatively simple API.

* The driver is expected to be able to "announce" new ticks to the
  kernel via the :c:func:`sys_clock_announce` call, which passes an integer
  number of ticks that have elapsed since the last announce call (or
  system boot).  These calls can occur at any time, but the driver is
  expected to attempt to ensure (to the extent practical given
  interrupt latency interactions) that they occur near tick boundaries
  (i.e. not "halfway through" a tick), and most importantly that they
  be correct over time and subject to minimal skew vs. other counters
  and real world time.

* The driver is expected to provide a :c:func:`sys_clock_set_timeout` call
  to the kernel which indicates how many ticks may elapse before the
  kernel must receive an announce call to trigger registered timeouts.
  It is legal to announce new ticks before that moment (though they
  must be correct) but delay after that will cause events to be
  missed.  Note that the timeout value passed here is in a delta from
  current time, but that does not absolve the driver of the
  requirement to provide ticks at a steady rate over time.  Naive
  implementations of this function are subject to bugs where the
  fractional tick gets "reset" incorrectly and causes clock skew.

* The driver is expected to provide a :c:func:`sys_clock_elapsed` call which
  provides a current indication of how many ticks have elapsed (as
  compared to a real world clock) since the last call to
  :c:func:`sys_clock_announce`, which the kernel needs to test newly
  arriving timeouts for expiration.

Timer Driver Locking
====================

The kernel exposes a unified timer lock via :c:func:`sys_clock_lock` and
:c:func:`sys_clock_unlock`.  This lock protects both the kernel's internal
tick accounting (``curr_tick``, the timeout queue) and any driver-private
state that must be consistent with it (e.g. a hardware cycle counter
baseline).

Timer drivers that maintain internal state should acquire this lock at
the start of their ISR, update their hardware state, then pass the lock
key to :c:func:`sys_clock_announce_locked` which consumes it.  This
ensures that the driver's cycle counter baseline and the kernel's
``curr_tick`` are always updated under the same lock, eliminating race
conditions that can arise on SMP systems (or, less commonly, on UP
systems where higher-priority ISRs need consistent realtime references)
when two separate locks are used.

The driver-provided callbacks :c:func:`sys_clock_set_timeout` and
:c:func:`sys_clock_elapsed` are always invoked by the kernel with this
lock already held.

For backward compatibility, :c:func:`sys_clock_announce` remains
available and acquires the lock internally.  New and migrated drivers
should prefer the :c:func:`sys_clock_lock` /
:c:func:`sys_clock_announce_locked` pattern.

Note that a natural implementation of this API results in a "tickless"
kernel, which receives and processes timer interrupts only for
registered events, relying on programmable hardware counters to
provide irregular interrupts.  But a traditional, "ticked" or "dumb"
counter driver can be trivially implemented also:

* The driver can receive interrupts at a regular rate corresponding to
  the OS tick rate, calling :c:func:`sys_clock_announce` with an argument of one
  each time.

* The driver can ignore calls to :c:func:`sys_clock_set_timeout`, as every
  tick will be announced regardless of timeout status.

* The driver can return zero for every call to :c:func:`sys_clock_elapsed`
  as no more than one tick can be detected as having elapsed (because
  otherwise an interrupt would have been received).

SMP Details
===========

In general, the timer API described above does not change when run in
a multiprocessor context.  The kernel will internally synchronize all
access appropriately, and ensure that all critical sections are small
and minimal.  But some notes are important to detail:

* Zephyr is agnostic about which CPU services timer interrupts.  It is
  not illegal (though probably undesirable in some circumstances) to
  have every timer interrupt handled on a single processor.  Existing
  SMP architectures implement symmetric timer drivers.

* The :c:func:`sys_clock_announce` call is expected to be globally
  synchronized at the driver level.  The kernel does not do any
  per-CPU tracking, and expects that if two timer interrupts fire near
  simultaneously, that only one will provide the current tick count to
  the timing subsystem.  The other may legally provide a tick count of
  zero if no ticks have elapsed.  It should not "skip" the announce
  call because of timeslicing requirements (see the time slicing notes in
  :ref:`kernel_timing`).

* Some SMP hardware uses a single, global timer device, others use a
  per-CPU counter.  The complexity here (for example: ensuring counter
  synchronization between CPUs) is expected to be managed by the
  driver, not the kernel.

* The next timeout value passed back to the driver via
  :c:func:`sys_clock_set_timeout` is done identically for every CPU.
  So by default, every CPU will see simultaneous timer interrupts for
  every event, even though by definition only one of them should see a
  non-zero ticks argument to :c:func:`sys_clock_announce`.  This is probably
  a correct default for timing sensitive applications (because it
  minimizes the chance that an errant ISR or interrupt lock will delay
  a timeout), but may be a performance problem in some cases.  The
  current design expects that any such optimization is the
  responsibility of the timer driver.
