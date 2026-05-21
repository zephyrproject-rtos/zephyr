.. _cpu_workload:

CPU Workload
############

Overview
********

The CPU Workload subsystem provides a unified CPU work observation layer for
Zephyr. It describes CPU work in cycles and exposes the work that has recently
executed, the runnable work that is currently visible, and the work that has
arrived since the previous query. Callers can use the subsystem either to obtain
a single workload estimate or to inspect the signals that contributed to that
estimate.

The subsystem provides four workload views:

* runtime history, which reports recently executed non-idle CPU cycles;
* ready backlog, which estimates cycles for threads that are runnable at the
  time of the query;
* arrival workload, which estimates cycles for wakeup events recorded since the
  previous query; and
* unified estimate, which combines the available views into one cycle count.

All workload values are reported in cycles. This lets callers compare the
different views without knowing the details of the scheduler statistics,
thread-state scan, or wakeup attribution used to build each result.

The subsystem also maintains per-thread burst profiles. A burst profile is not
reported as a separate public workload view, but it is the cost model used by
ready backlog and arrival workload to convert threads or wakeup events into
cycle estimates.

Workload Sources
****************

Runtime History
===============

Runtime history is based on CPU runtime statistics. Each successful call to
:c:func:`cpu_workload_history_get` reports the delta since the previous history
query for that CPU.

The first query only establishes a baseline and returns ``-EAGAIN``. Later
queries report:

* non-idle cycles observed in the history window;
* total runtime-stat cycles in the same window;
* the window duration converted to microseconds;
* load as a percentage of the history window; and
* confidence for the history sample.

Runtime history is useful as a record of work that has already happened. For
example, a periodic thread may finish before the next query. In that case, the
ready queue may already be empty, but runtime history still shows that the CPU
was recently busy.

Thread Burst Profile
====================

The thread burst profile estimates how many cycles a thread usually consumes in
one scheduling burst. It is built from per-thread runtime usage windows. When a
thread has new runtime usage windows, the subsystem computes a new burst sample:

.. code-block:: text

   burst sample = delta(total_cycles) / delta(window_count)

The sample is then folded into the maintained average for that thread with an
EWMA. EWMA stands for Exponentially Weighted Moving Average. It combines the
newest sample with the previous profile:

.. code-block:: text

   profile_new = alpha * sample + (1 - alpha) * profile_old

Here, ``alpha`` is the smoothing factor. It is needed because a thread's burst
cost is neither perfectly constant nor completely random. Using only the newest
sample would make the profile too sensitive to a single unusual run, while
using only a long-term average would make it too slow to follow real workload
behavior changes. EWMA keeps both pieces of information: the newest sample
reflects recently observed behavior, and the previous profile preserves the
behavior model accumulated so far.

A larger ``alpha`` gives more weight to the newest sample, so the profile
follows recent runtime behavior more quickly. A smaller ``alpha`` gives more
weight to the previous profile, so the result is smoother and less sensitive to
a single abnormal burst. The purpose of using EWMA for the thread burst profile
is to balance adaptation to workload behavior changes against overreaction to
occasional jitter.

Each profile also has a confidence value from 0 to 100. Profiles with no samples
report 0 confidence. As a thread accumulates scheduling-window samples, the
confidence increases in coarse buckets until it reaches 100. Ready backlog and
arrival workload use this confidence to decide how much trust to place in the
cycles estimated from the profile.

Thread burst profiles are enabled with
:kconfig:option:`CONFIG_CPU_WORKLOAD_THREAD_PROFILE`. The cache size is
controlled by
:kconfig:option:`CONFIG_CPU_WORKLOAD_THREAD_PROFILE_CACHE_SIZE`, and the EWMA
responsiveness is controlled by
:kconfig:option:`CONFIG_CPU_WORKLOAD_THREAD_PROFILE_EWMA_SHIFT`.

Ready Backlog
=============

Ready backlog is based on the threads that are runnable on the selected CPU at
the time of the query. The query reports the amount of visible runnable work
that has not yet completed.

A runnable thread does not directly say how many cycles it will need next.
Ready backlog also uses the per-thread burst profile to convert runnable
threads into cycles:

.. code-block:: text

   ready_backlog_cycles += thread_burst_profile

:c:func:`cpu_workload_ready_backlog_get` scans runnable threads and adds the
burst profile cycles for threads with usable profiles. Runnable threads without
usable profiles are still counted in ``runnable_threads``, but they do not
contribute cycles to ``ready_backlog_cycles``. The confidence value is therefore
conservative: missing or low-confidence profiles reduce the trust in the
backlog result.

Arrival Workload
================

Arrival workload is based on per-thread arrival statistics. It records wakeup
events that happened since the previous arrival query. A recorded arrival means
that a thread moved from a non-ready state to runnable through a scheduler
wakeup path, such as timeout expiry, synchronization object readiness, or an
explicit wakeup.

Arrival is an event ledger, not a current thread-state snapshot. By the time
:c:func:`cpu_workload_arrival_get` is called, a thread that arrived earlier in
the window may still be runnable, may be running, or may already have completed
and blocked again. The arrival query reports that the work arrived during the
window and then clears the arrival window.

Arrival workload also uses the per-thread burst profile to convert events into
cycles:

.. code-block:: text

   expected_arrival_cycles += arrival_count * thread_burst_profile

Arrival and ready backlog can therefore describe the same thread from two
different angles. If a thread was woken recently and is still runnable when the
query happens, it can appear in both the arrival ledger and the ready-backlog
snapshot. The subsystem keeps the individual fields and source mask so callers
can decide how to interpret or de-weight overlapping signals.

Unified Estimate
****************

The unified estimate is available through
:c:func:`cpu_workload_estimate_get`. It combines runtime history, ready backlog,
and arrival workload into one cycle count for callers that want a single value.

The estimate first builds a forward-looking workload from currently visible
queued work and recently recorded arrivals:

.. code-block:: text

   forward_cycles = ready_backlog_cycles + expected_arrival_cycles

It then uses recent runtime history as a conservative floor:

.. code-block:: text

   estimated_cycles = max(history_cycles, forward_cycles)

Runtime history is not added directly to the forward-looking signal because it
describes work that has already run. Taking the maximum helps avoid
underestimating sustained load while avoiding direct double-counting of the
history window.

The unified result still exposes the individual history, backlog, and arrival
fields, along with the source mask. Callers that need more specific behavior can
inspect those fields instead of treating the estimate as an opaque value.

Configuration Options
*********************

Enable the subsystem with :kconfig:option:`CONFIG_CPU_WORKLOAD`.

Related configuration options include:

* :kconfig:option:`CONFIG_CPU_WORKLOAD_HISTORY`
* :kconfig:option:`CONFIG_CPU_WORKLOAD_THREAD_PROFILE`
* :kconfig:option:`CONFIG_CPU_WORKLOAD_THREAD_PROFILE_EWMA_SHIFT`
* :kconfig:option:`CONFIG_CPU_WORKLOAD_THREAD_PROFILE_CACHE_SIZE`
* :kconfig:option:`CONFIG_CPU_WORKLOAD_READY_BACKLOG`
* :kconfig:option:`CONFIG_CPU_WORKLOAD_READY_BACKLOG_MIN_CONFIDENCE`
* :kconfig:option:`CONFIG_CPU_WORKLOAD_ARRIVAL`
* :kconfig:option:`CONFIG_CPU_WORKLOAD_ESTIMATE`

The runtime-history, ready-backlog, arrival, and unified-estimate options select
the scheduler statistics and thread monitoring support they require.

Relationship to CPU Load
************************

The CPU Load API reports a percentage for the interval between calls. CPU
Workload reports cycle counts and keeps separate fields for history, runnable
backlog, and arrivals. Existing code that only needs a percentage can continue
to use CPU Load. New code that needs to explain where CPU work came from should
use CPU Workload directly.

API Reference
*************

.. doxygengroup:: subsys_cpu_workload
