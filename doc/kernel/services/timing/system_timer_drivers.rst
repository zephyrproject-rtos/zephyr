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

* The driver may optionally provide a :c:func:`sys_clock_no_timeout` call.
  The kernel invokes it in place of :c:func:`sys_clock_set_timeout` when no
  timeout is pending, that is when the timeout queue is empty, and
  :kconfig:option:`CONFIG_SYSTEM_CLOCK_SLOPPY_IDLE` is enabled.

  No tick announcement is forthcoming and the system does not care about precise
  uptime keeping, so the driver may do something to save resources.  Normal
  operation resumes at the next :c:func:`sys_clock_set_timeout`; unlike
  :c:func:`sys_clock_disable`, this is not a teardown.

  The timer *counter* must not be stopped.  :c:func:`sys_clock_cycle_get_32`
  and :c:func:`sys_clock_cycle_get_64` must keep counting up as if the call had
  never happened.

  .. note::

     The CPU keeps running with an empty timeout queue, for instance because
     the ready queue still holds threads, or because an interrupt fires.  Those
     threads and ISRs may call :c:func:`k_cycle_get_32` or
     :c:func:`k_busy_wait`.  That limits what an implementation may do: masking
     the timer interrupt is safe, gating the timer's clock most likely is not.
     Exactly what is safe is hardware specific.

  The hook is optional.  Without it, :c:func:`sys_clock_set_timeout` is asked
  for the longest wait it can express, ``UINT32_MAX`` ticks.  That is
  numerically what ``K_TICKS_FOREVER`` was here, the long-standing "no deadline"
  signal, so a driver that has not been converted keeps working.

* The driver may optionally provide a :c:func:`sys_clock_idle_enter` call,
  which the power-management path uses in place of
  :c:func:`sys_clock_set_timeout` when the CPU is about to enter low-power
  idle.  It receives the number of ticks until the next expected wakeup, or
  ``SYS_CLOCK_IDLE_FOREVER`` when there is none and the uptime accounting may
  drift.

  A driver that hands off to a low-power wakeup timer, or otherwise
  reconfigures itself for sleep, does so here.  Told ``SYS_CLOCK_IDLE_FOREVER``
  it may also stop its time base, which :c:func:`sys_clock_no_timeout` does not
  allow, because the CPU is on its way out and :c:func:`sys_clock_idle_exit` is
  guaranteed to run on the way back in.  Only the calling CPU goes idle, so a
  driver whose time base is shared between CPUs must ensure only the last CPU
  going idle stops the clock.

  The hook is optional.  Without it, :c:func:`sys_clock_set_timeout` is called
  with its deprecated ``idle`` argument set to ``true``, so a driver still
  keying its low-power handling on that argument keeps working.

The last three entry points divide the four states a system can be in:

.. list-table::
   :header-rows: 1
   :widths: 15 45 40

   * -
     - nothing pending
     - something pending
   * - **running**
     - ``sys_clock_no_timeout()``
     - ``sys_clock_set_timeout(ticks)``
   * - **idle**
     - ``sys_clock_idle_enter(SYS_CLOCK_IDLE_FOREVER)``
     - ``sys_clock_idle_enter(ticks)``

Without :kconfig:option:`CONFIG_SYSTEM_CLOCK_SLOPPY_IDLE` the left column never
arises: the kernel keeps a synthetic deadline armed so the uptime stays exact,
and a driver only ever sees the right one.

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
