.. _clock_monitor_api:

Clock Monitor
#############

Overview
********

The clock monitor API provides access to hardware peripherals that observe a
clock signal at runtime and report when its frequency drifts outside expected
bounds or stops entirely. It is intended for functional-safety and
diagnostic use cases — detecting failed oscillators, lost reference clocks,
or out-of-spec frequency drift on critical clock trees.

Operating Modes
***************

Both modes share one lifecycle: configure with
:c:func:`clock_monitor_configure`, begin operation with
:c:func:`clock_monitor_start`, end it with :c:func:`clock_monitor_stop`.

The API exposes two operating modes:

``CLOCK_MONITOR_MODE_WINDOW``
   Continuous threshold check. The hardware compares the monitored clock's
   frequency against programmable high and low bounds derived from
   :c:member:`clock_monitor_window_cfg.expected_hz` and
   :c:member:`clock_monitor_window_cfg.tolerance_ppm`. Threshold crossings
   are delivered asynchronously through the user callback installed at
   configure time.

``CLOCK_MONITOR_MODE_MEASURE``
   One frequency measurement per :c:func:`clock_monitor_start`, delivered
   through the configure-time callback (``CLOCK_MONITOR_EVT_MEASURE_DONE``
   plus the measured value in Hz). The device automatically returns to the
   configured (stopped) state before the callback runs, so no
   :c:func:`clock_monitor_stop` is needed on the happy path. To repeat
   the measurement, call :c:func:`clock_monitor_start` again from the callback —
   both :c:func:`clock_monitor_start` and :c:func:`clock_monitor_stop` are
   ISR-safe (the same idiom as re-arming a counter alarm from its
   callback).

For MEASURE mode, the API provides no blocking wait: the application
owns the timeout. The usual pattern is to wait on a semaphore given from
the callback with an application-chosen timeout and call
:c:func:`clock_monitor_stop` on the timeout path to abort the in-flight
measurement. Alternatively, the most recent result can be polled with
:c:func:`clock_monitor_get_rate` — also the retrieval path for user-mode
threads, where callbacks may not be installed.

Events
******

Events are delivered as a bitmask in :c:member:`clock_monitor_event_data.events`:

* ``CLOCK_MONITOR_EVT_FREQ_HIGH`` — monitored frequency exceeded the
  upper threshold (WINDOW mode).
* ``CLOCK_MONITOR_EVT_FREQ_LOW`` — monitored frequency fell below the
  lower threshold (WINDOW mode).
* ``CLOCK_MONITOR_EVT_CLOCK_LOST`` — monitored clock stopped producing
  edges within the measurement window (MEASURE mode hardware failure).
* ``CLOCK_MONITOR_EVT_MEASURE_DONE`` — measurement completed successfully
  (MEASURE mode); :c:member:`clock_monitor_event_data.measured_hz`
  holds the result.

The configure-time callback is the only event delivery path. For MEASURE
mode the most recent completed result additionally remains retrievable by
polling :c:func:`clock_monitor_get_rate`. WINDOW-mode user-mode observers,
where callbacks may not be installed, receive events via a
supervisor-side relay (e.g. :c:struct:`k_msgq` fed from the callback).

Configuration
*************

A clock monitor must be configured via :c:func:`clock_monitor_configure`
before it is started. The configuration carries the operating mode, the
mode-specific parameters (expected frequency, tolerance, measurement
window) and an optional asynchronous callback. Configuring is only
possible while the monitor is stopped; reconfiguration is done by calling
:c:func:`clock_monitor_configure` again with a new mode/parameter set —
there is no separate teardown operation. See :c:func:`clock_monitor_configure`
in the `API Reference`_ for the full list of return codes.

Related configuration options:

* :kconfig:option:`CONFIG_CLOCK_MONITOR`

API Reference
*************

.. doxygengroup:: clock_monitor_interface
