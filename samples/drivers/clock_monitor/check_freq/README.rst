.. zephyr:code-sample:: clock-monitor-check-freq
   :name: Clock Monitor Frequency Check
   :relevant-api: clock_monitor_interface

   Continuously check a clock against programmable frequency thresholds.

Overview
********

This sample uses the :ref:`clock monitor API <clock_monitor_api>` in
:c:enumerator:`CLOCK_MONITOR_MODE_WINDOW` mode: the hardware continuously compares a clock against
programmable thresholds and reports every crossing through a callback, which logs whether the
clock ran too fast or too slow. The main loop only prints a periodic heartbeat.

The window is centred on ``CONFIG_SAMPLE_EXPECTED_HZ``, widened by
``CONFIG_SAMPLE_TOLERANCE_PPM`` and refreshed every ``CONFIG_SAMPLE_WINDOW_NS``. Leaving the
expected frequency at ``0`` derives the centre from the clock's actual rate through the
:ref:`clock control API <clock_control_api>`.

Requirements
************

* A clock monitor exposed through the ``clock-monitor`` devicetree alias.
* FREQME-based boards also need
  :zephyr_file:`samples/drivers/clock_monitor/clock_monitor.overlay`.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/clock_monitor/check_freq
   :board: frdm_mcxe31b
   :goals: build flash
   :compact:

On a board using the FREQME peripheral, add the shared overlay:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/clock_monitor/check_freq
   :board: frdm_mcxn236
   :goals: build flash
   :gen-args: -DEXTRA_DTC_OVERLAY_FILE="../clock_monitor.overlay"
   :compact:

Sample Output
=============

.. code-block:: console

   [00:00:00.000,000] <inf> sample: clock check running (expected ~0 Hz, tolerance +/-50000 ppm, window 1000000 ns)
   [00:00:05.000,000] <inf> sample: heartbeat: clock check running
   [00:00:10.000,000] <inf> sample: heartbeat: clock check running

A clock drifting outside the window, or stopping, is reported from the callback:

.. code-block:: console

   [00:00:12.345,000] <err> sample: [cmu-fc@2bc000] monitored clock below lower threshold (or lost)
