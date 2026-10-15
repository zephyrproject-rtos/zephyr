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

API Reference
*************

.. doxygengroup:: task_wdt_api
