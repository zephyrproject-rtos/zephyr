.. _task_wdt_api:

Task Watchdog
#############

Overview
********

Many microcontrollers feature a hardware watchdog timer peripheral. Its purpose
is to trigger an action (usually a system reset) in case of severe software
malfunctions. Once initialized, the watchdog timer has to be restarted ("fed")
in regular intervals to prevent it from timing out. If the software got stuck
and does not manage to feed the watchdog anymore, the corrective action is
triggered to bring the system back to normal operation.

In real-time operating systems with multiple tasks running in parallel, a
single watchdog instance may not be sufficient anymore, as it can be used for
only one task. This software watchdog based on kernel timers provides a method
to supervise multiple threads or tasks (called watchdog channels).

An existing hardware watchdog can be used as an optional fallback if the task
watchdog itself or the scheduler has a malfunction.

The task watchdog uses a kernel timer as its backend. If configured properly,
the timer ISR is never actually called during normal operation, as the timer is
continuously updated in the feed calls.

It's currently not possible to have multiple instances of task watchdogs.
Instead, the task watchdog API can be accessed globally to add or delete new
channels without passing around a context or device pointer in the firmware.

The maximum number of channels is predefined via Kconfig and should be adjusted
to match exactly the number of channels required by the application.

Thread Information
******************

With :kconfig:option:`CONFIG_TASK_WDT_THREAD_INFO` enabled, each channel
records the thread owning it. By default, this is the thread which fed the
channel last. If the channel is fed on behalf of another thread, the owner can
be set explicitly with :c:func:`task_wdt_channel_thread_set`.

When a channel times out, the task watchdog logs the owner thread's name,
state, priority and unused stack space before calling the channel callback or
resetting the system. :c:func:`task_wdt_channel_info_get` and the
``task_wdt list`` shell command show the owner, period and time left of each
channel.

If :kconfig:option:`CONFIG_TASK_WDT_COREDUMP` is enabled as well, a core dump
is created with the owner thread as the faulting thread before the system is
reset, which helps to find out where the thread was stuck.

Fault Reasons
*************

With :kconfig:option:`CONFIG_TASK_WDT_FAULT_REASONS` enabled, a channel added
with :c:func:`task_wdt_add_ext` has a callback which is told the reason of the
fault (:c:enum:`task_wdt_fault_reason`) and the thread owning the channel.

A channel can also check that it is not fed too often. After
:c:func:`task_wdt_window_set`, feeding the channel again sooner than the given
minimum interval after the previous feed is reported as
:c:enumerator:`TASK_WDT_FAULT_TOO_EARLY`. This detects a thread stuck in a loop
which keeps feeding its channel. The fault is reported from
:c:func:`task_wdt_feed`, in the context of the feeding thread, while timeouts
are reported from the timer interrupt. Channels added with
:c:func:`task_wdt_add` handle this fault like a timeout.

Checkpoint Supervision
**********************

With :kconfig:option:`CONFIG_TASK_WDT_SUPERVISION` enabled, the task watchdog
can also check the program flow of an entity, for example the main loop of a
thread. The entity is defined at build time with
:c:macro:`TASK_WDT_SUPERVISION_DEFINE`, from a list of checkpoints and the
checkpoints allowed to follow each of them, and a list of deadlines between
two checkpoints. The application reports each checkpoint it reaches with
:c:func:`task_wdt_checkpoint`.

.. code-block:: c

   enum { CP_START, CP_WORK, CP_END };

   static const uint32_t next[] = {
           [CP_START] = BIT(CP_WORK),
           [CP_WORK] = BIT(CP_WORK) | BIT(CP_END),
           [CP_END] = BIT(CP_START),
   };

   /* CP_END has to be reached between 10 ms and 50 ms after CP_START */
   static const struct task_wdt_deadline deadlines[] = {
           {.start = CP_START, .end = CP_END, .min_ms = 10, .max_ms = 50},
   };

   TASK_WDT_SUPERVISION_DEFINE(main_loop, next, deadlines);

After :c:func:`task_wdt_supervision_start`, the faults of the entity are
reported on a channel which is still fed as usual:

* :c:enumerator:`TASK_WDT_FAULT_SEQUENCE` if a checkpoint is not allowed after
  the previous one,
* :c:enumerator:`TASK_WDT_FAULT_DEADLINE_MIN` if the end of a deadline is reached
  too early,
* :c:enumerator:`TASK_WDT_FAULT_DEADLINE_MAX` if the end of a deadline is not
  reached in time.

The first two are reported from :c:func:`task_wdt_checkpoint`, the last one
from the timer interrupt. Deadlines use the same kernel timer as the channels,
no additional timer is needed. Only one deadline of an entity runs at a time,
and :c:func:`task_wdt_resume` cancels the running deadlines.

.. note::

   Checkpoint supervision provides alive, deadline and logical program flow
   monitoring mechanisms as required by functional safety standards. Using it
   does not make an application compliant with such a standard.

Reset Information
*****************

With :kconfig:option:`CONFIG_TASK_WDT_RESET_INFO` enabled, the task watchdog
stores a record before the system is reset because a channel timed out. The
record is also stored if the channel callback resets the system. After the
reset, :c:func:`task_wdt_reset_info_get` returns the channel, the name of its
owner thread (with :kconfig:option:`CONFIG_TASK_WDT_THREAD_INFO` and
:kconfig:option:`CONFIG_THREAD_NAME`), the uptime at the time of the reset and
the number of consecutive task watchdog resets. A reset caused by feeding a
channel too early is reported as :c:enumerator:`TASK_WDT_RESET_TOO_EARLY`, and
the checkpoint supervision faults have their own reset reasons as well. The
count restarts from zero
after any other reset or a call to :c:func:`task_wdt_reset_info_clear`.

The record is written from the timer interrupt. By default, it is kept in RAM
which is not cleared at boot (:kconfig:option:`CONFIG_TASK_WDT_RESET_INFO_NOINIT`).
Alternatively, it can be kept in a :ref:`retention <retention_api>` area which
is selected with the ``zephyr,task-wdt-reset-info`` chosen node
(:kconfig:option:`CONFIG_TASK_WDT_RESET_INFO_RETENTION`). As the retention API
is used from the interrupt, its mutexes have to be disabled with
:kconfig:option:`CONFIG_RETENTION_MUTEX_FORCE_DISABLE` and
:kconfig:option:`CONFIG_RETAINED_MEM_MUTEX_FORCE_DISABLE`. The retained memory
driver must not block or sleep either, which is the case for the RAM based
drivers but not for the drivers accessing an external device over a bus.

If the hardware watchdog fallback resets the system, the task watchdog cannot
store a record. With :kconfig:option:`CONFIG_TASK_WDT_RESET_INFO_HW_FALLBACK`,
a watchdog reset reported by :c:func:`hwinfo_get_reset_cause` without a record
is returned as :c:enumerator:`TASK_WDT_RESET_HW_FALLBACK`, which means the whole
system was stuck.

Configuration Options
*********************

Related configuration options can be found under
:zephyr_file:`subsys/task_wdt/Kconfig`.

* :kconfig:option:`CONFIG_TASK_WDT`

* :kconfig:option:`CONFIG_TASK_WDT_CHANNELS`

* :kconfig:option:`CONFIG_TASK_WDT_HW_FALLBACK`

* :kconfig:option:`CONFIG_TASK_WDT_MIN_TIMEOUT`

* :kconfig:option:`CONFIG_TASK_WDT_HW_FALLBACK_DELAY`

* :kconfig:option:`CONFIG_TASK_WDT_THREAD_INFO`

* :kconfig:option:`CONFIG_TASK_WDT_COREDUMP`

* :kconfig:option:`CONFIG_TASK_WDT_FAULT_REASONS`

* :kconfig:option:`CONFIG_TASK_WDT_SUPERVISION`

* :kconfig:option:`CONFIG_TASK_WDT_RESET_INFO`

* :kconfig:option:`CONFIG_TASK_WDT_RESET_INFO_NOINIT`

* :kconfig:option:`CONFIG_TASK_WDT_RESET_INFO_RETENTION`

* :kconfig:option:`CONFIG_TASK_WDT_RESET_INFO_HW_FALLBACK`

API Reference
*************

.. doxygengroup:: task_wdt_api
