.. Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
.. SPDX-License-Identifier: Apache-2.0

.. _kernel_critical_section_monitor:

Critical Section Monitor
########################

The Critical Section Monitor records the maximum IRQ-locked, spinlock wait,
and spinlock hold times per CPU. It does not print, allocate memory, or take
another lock in the measured path.

Measured intervals
******************

* **Interrupts locked** -- time spent with interrupts locked through the
  public :c:func:`irq_lock`, :c:func:`k_irq_lock`, or spinlock APIs. Nested
  locking counts as one outer interval. Idle time and time spent waiting
  to run again after a context switch are excluded.
* **Spinlock wait** -- time from an SMP lock attempt to successful acquisition.
  This is zero on uniprocessor systems.
* **Spinlock hold** -- time from acquisition to release.

Only completed intervals are recorded. The monitor does not detect a lockup
while it is still in progress or measure hardware interrupt response time.

Configuration
*************

Enable :kconfig:option:`CONFIG_CRITICAL_SECTION_MONITOR`. It requires a system
timer with a lock-free 32-bit cycle counter. SMP builds also require the
compiler-builtin atomic backend. To read statistics from the shell, add these
options to your application configuration:

.. code-block:: cfg

   CONFIG_CRITICAL_SECTION_MONITOR=y
   CONFIG_SHELL=y
   CONFIG_KERNEL_SHELL=y

:kconfig:option:`CONFIG_CRITICAL_SECTION_MONITOR_MAX_SPINLOCKS` sets the
number of spinlock holds tracked at once per CPU (default: four). If all slots
are in use, extra holds are skipped and counted as tracking overflows. IRQ
and spinlock-wait measurements continue.

Storage is fixed per CPU. On UP builds without spinlock validation,
:kconfig:option:`CONFIG_NONZERO_SPINLOCK_SIZE` gives otherwise empty locks
distinct addresses, increasing their size and possibly structure padding.

Reading statistics
******************

Run:

.. code-block:: console

   uart:~$ kernel critical

The command shows each CPU's maxima in cycles and nanoseconds, with the caller,
spinlock, and thread addresses, an ISR-context flag, and the tracking overflow
count. A zero duration means no interval has been recorded for that class.
Addresses are diagnostic identifiers; the objects may no longer exist and must
not be dereferenced.

Reading does not clear statistics. Each CPU's snapshot is consistent, but CPUs
are sampled separately. If one CPU's statistics stay busy, the command reports
that CPU as busy and continues with the others; run it again to retry. Shell
activity can itself contribute new maxima. Collection also works without a shell.

Behavior and limitations
************************

* Monitoring starts during kernel initialization; earlier activity is not recorded.
* Intervals must be shorter than ``2^32`` hardware cycles. Counter wrap can make
  longer intervals appear shorter.
* Direct architecture IRQ calls are not tracked. Do not mix public and
  architecture lock/unlock APIs within a pair; this can produce incorrect timing.
* Compiler and linker optimizations may make caller addresses hard to resolve.

Monitoring adds overhead, and reported durations include some of it. Use the
monitor to find long critical sections, not to time short instruction sequences.
Compare the same workload on target hardware with monitoring enabled and disabled.
