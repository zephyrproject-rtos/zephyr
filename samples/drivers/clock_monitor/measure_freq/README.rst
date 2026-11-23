.. zephyr:code-sample:: clock-monitor-measure-freq
   :name: Clock Monitor Frequency Meter
   :relevant-api: clock_monitor_interface

   Measure the frequency of a clock once and print the result.

Overview
********

This sample uses the :ref:`clock monitor API <clock_monitor_api>` in
:c:enumerator:`CLOCK_MONITOR_MODE_MEASURE` mode, which performs one measurement per
:c:func:`clock_monitor_start`. It shows the idiomatic wait pattern: the callback gives a
semaphore, ``main()`` waits on it with its own timeout, and the result is read back with
:c:func:`clock_monitor_get_rate`.

The API has no blocking wait, so the timeout belongs to the application; on timeout the sample
calls :c:func:`clock_monitor_stop` to abort the measurement. The happy path needs no stop, as the
device disarms itself before the callback runs. The measurement window is set using
``CONFIG_SAMPLE_WINDOW_NS``.

Requirements
************

* A clock monitor exposed through the ``clock-meter`` devicetree alias.
* FREQME-based boards also need
  :zephyr_file:`samples/drivers/clock_monitor/clock_monitor.overlay`.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/clock_monitor/measure_freq
   :board: frdm_mcxe31b
   :goals: build flash
   :compact:

On a board using the FREQME peripheral, add the shared overlay:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/clock_monitor/measure_freq
   :board: frdm_mcxn236
   :goals: build flash
   :gen-args: -DEXTRA_DTC_OVERLAY_FILE="../clock_monitor.overlay"
   :compact:

Sample Output
=============

.. code-block:: console

   [00:00:00.000,000] <inf> sample: cmu-fm@2bc020 configured in MEASURE mode, window = 1000000 ns
   [00:00:00.001,000] <inf> sample: Measured frequency = 48000000 Hz

If the monitored clock stops producing edges, the driver reports the loss instead:

.. code-block:: console

   [00:00:00.001,000] <wrn> sample: monitored clock lost (FMTO-class event)
