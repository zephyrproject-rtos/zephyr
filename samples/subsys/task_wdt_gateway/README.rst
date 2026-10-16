.. zephyr:code-sample:: task-wdt-gateway
   :name: Task watchdog sensor gateway
   :relevant-api: task_wdt_api boot_mode_interface

   Detect, recover from and contain faults of a sensor gateway with the task watchdog.

Overview
********

A watchdog which only resets the system when a thread stops feeding it is a
blunt tool. This sample shows how the :ref:`task watchdog <task_wdt_api>`
detects more kinds of faults, recovers from them without a reset whenever
possible, and still keeps a device from resetting forever when it cannot.

The sample is a sensor gateway made only of software components, so it runs in
QEMU:

* a ``sampler`` thread reads a simulated sensor every 100 ms, filters the
  reading with a calibration factor kept in a RAM area surviving resets (which
  stands in for flash), and queues it,
* an ``uplink`` thread sends the queued readings in batches every 200 ms over
  a simulated modem.

Each thread has its own channel. The main thread then injects the faults such
a device sees in the field, one after the other:

The modem stops answering
   The uplink blocks forever waiting for an acknowledgment and its channel
   times out. A network outage is expected from time to time, so the uplink
   channel is in a group with the :c:enumerator:`TASK_WDT_ACTION_RECOVER`
   action: its recovery function restarts the uplink thread from the task
   watchdog recovery work queue, and the rest of the gateway keeps running.

The uplink retries a transmission forever
   A buggy retry loop still feeds the uplink channel, so a plain watchdog never
   notices it. The channel has a minimum feed interval set with
   :c:func:`task_wdt_window_set`, and feeding it every few milliseconds is
   reported as :c:enumerator:`TASK_WDT_FAULT_TOO_EARLY`.

The filter takes too long
   The acquisition goes through the checkpoints acquire, filter and publish,
   supervised with :c:macro:`TASK_WDT_SUPERVISION_DEFINE`. A glitch makes the
   filter take 200 ms although the acquisition must take at most 50 ms. The
   deadline is reported as missed long before the 500 ms channel of the sampler
   would time out, and the sampler is restarted.

A configuration update stores a bad calibration
   The sampler hangs in the filter again. The acquisition is not expected to
   fail, so its group has an escalation: this second fault within 10 s resets
   the system. The bad calibration survives the reset, so the sampler hangs
   again after the reboot and its restart does not help either. After the
   second consecutive task watchdog reset, the task watchdog sets a boot mode
   (:kconfig:option:`CONFIG_TASK_WDT_SAFE_MODE`) and the gateway boots into a
   safe mode instead of resetting forever. The safe mode does not start the
   acquisition, restores the factory calibration and reboots into the normal
   mode.

At each boot, the gateway reads the cause of the last reset with
:c:func:`task_wdt_reset_info_get`, which tells which fault of which thread
reset the system, and how many times in a row.

With :kconfig:option:`CONFIG_TASK_WDT_THREAD_INFO` enabled, the task watchdog
also logs the state of the thread owning a faulty channel.

Building and Running
********************

The sample runs on :zephyr:board:`qemu_cortex_m3`, which keeps the boot mode
in a retention area defined in
:zephyr_file:`samples/subsys/task_wdt_gateway/boards/qemu_cortex_m3.overlay`:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/task_wdt_gateway
   :board: qemu_cortex_m3
   :goals: build run
   :compact:

Without a boot mode, the sample enters the safe mode based on the number of
consecutive resets returned by :c:func:`task_wdt_reset_info_get` instead.

Sample output
=============

.. code-block:: console

   *** Booting Zephyr OS build ... ***
   Sensor gateway sample
   First boot, using the factory calibration
   Last reset: not caused by the task watchdog
   Injecting: the modem stops answering
   [00:00:01.840,000] <err> task_wdt: Channel 1 timed out, last fed 1000 ms ago
   [00:00:01.840,000] <err> task_wdt: Owner thread 0x20000070 (uplink): pending, priority 6
   Recovery: restarting uplink (timeout)
   Injecting: the uplink retries a transmission forever
   [00:00:03.120,000] <err> task_wdt: Channel 1 fed too early
   [00:00:03.120,000] <err> task_wdt: Owner thread 0x20000070 (uplink): queued, priority 6
   Recovery: restarting uplink (fed too early)
   Injecting: a sensor bus glitch slows down the filter
   [00:00:05.110,000] <err> task_wdt: Channel 0 checkpoint deadline missed
   [00:00:05.110,000] <err> task_wdt: Owner thread 0x20000128 (sampler): sleeping, priority 5
   Recovery: restarting sampler (deadline missed)
   Injecting: a configuration update with a bad calibration
   [00:00:07.140,000] <err> task_wdt: Channel 0 checkpoint deadline missed
   [00:00:07.140,000] <err> task_wdt: Owner thread 0x20000128 (sampler): sleeping, priority 5
   *** Booting Zephyr OS build ... ***
   Sensor gateway sample
   Last reset: task watchdog deadline missed, thread sampler, 1 consecutive
   [00:00:00.050,000] <err> task_wdt: Channel 0 checkpoint deadline missed
   [00:00:00.050,000] <err> task_wdt: Owner thread 0x20000128 (sampler): sleeping, priority 5
   Recovery: restarting sampler (deadline missed)
   [00:00:00.100,000] <err> task_wdt: Channel 0 checkpoint deadline missed
   [00:00:00.100,000] <err> task_wdt: Owner thread 0x20000128 (sampler): sleeping, priority 5
   *** Booting Zephyr OS build ... ***
   Sensor gateway sample
   Last reset: task watchdog deadline missed, thread sampler, 2 consecutive
   Safe mode: the gateway keeps resetting, the acquisition is not started
   Safe mode: restored the factory calibration
   Safe mode: rebooting into the normal mode
   *** Booting Zephyr OS build ... ***
   Sensor gateway sample
   Last reset: not caused by the task watchdog
   Gateway running normally, 18 readings sent
   Gateway running normally, 37 readings sent
