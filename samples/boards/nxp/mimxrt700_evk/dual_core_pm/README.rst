.. zephyr:code-sample:: nxp_rt700_dual_core_pm
   :name: RT700 EVK dual-core power management

   Exercise active, sleep, deep-sleep and deep-sleep-retention combinations on
   both RT700 EVK Cortex-M33 cores.

Overview
********

This sample builds two Zephyr images for the MIMXRT700-EVK:

* the main image runs on CM33 CPU0 (the Compute core) and boots CM33 CPU1
  through the existing ``CONFIG_SECOND_CORE_MCUX`` board initialization path;
* the remote image runs on CM33 CPU1 (the Sense core).

Both images enable Zephyr system power management and register a PM notifier
that counts state entry/exit events. CPU0 is the test orchestrator: it prints
an interactive menu on its console and, for the selected case, hands CPU1 a
mode over the MU (Messaging Unit) mailbox. The two cores then enter their
windows together through a small handshake:

#. CPU0 sends the mode command, CPU1 acknowledges that it is ready;
#. CPU0 sends ``GO`` to release CPU1;
#. each core runs its own measurement window for the configured number of
   seconds;
#. CPU1 reports that its window is complete.

The test matrix covers eight cases:

#. both cores active (baseline);
#. CPU0 sleep, CPU1 active;
#. CPU0 active, CPU1 sleep;
#. both cores sleep;
#. CPU0 deep-sleep, CPU1 active;
#. CPU0 active, CPU1 deep-sleep;
#. both cores deep-sleep;
#. CPU0 deep-sleep-retention (DSR), CPU1 deep-sleep.

The deeper PM states are locked out at startup, so a plain sleep window stays in
``PM_STATE_RUNTIME_IDLE``. Each deep window then unlocks exactly the state it
exercises for the duration of that window only:

* sleep windows: ``PM_STATE_RUNTIME_IDLE``;
* deep-sleep windows: ``PM_STATE_SUSPEND_TO_IDLE``;
* the DSR window: ``PM_STATE_STANDBY``.

Deep-sleep-retention is a dual-domain (FDSR) mode that only engages when both
the Compute and Sense domains request deep sleep, so case 8 parks CPU1 in deep
sleep while CPU0 enters DSR.

CPU0 and CPU1 each print to their own debug UART. The serial output is a coarse
software confirmation that both cores booted and that the PM entry/exit
callbacks fired; use the stable low-power windows for board current measurement.

Building and Running
********************

Build the CPU0 and CPU1 images together with sysbuild:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/mimxrt700_evk/dual_core_pm
   :board: mimxrt700_evk/mimxrt798s/cm33_cpu0
   :west-args: --sysbuild
   :goals: build flash
   :compact:

After reset, CPU0 prints the menu on its console. Type the digit of the case to
run (``1``-``8``); the selected case runs once and the menu is printed again.
For example, selecting case 1 on the CPU0 console:

.. code-block:: console

   RT700 dual-core PM sample: CPU0 started on mimxrt700_evk/mimxrt798s/cm33_cpu0

   RT700 dual-core PM menu:
     1: active baseline
     2: CPU0 sleep, CPU1 active
     3: CPU0 active, CPU1 sleep
     4: CPU0 and CPU1 sleep
     5: CPU0 deep sleep, CPU1 active
     6: CPU0 active, CPU1 deep sleep
     7: CPU0 and CPU1 deep sleep
     8: CPU0 deep sleep retention, CPU1 deep sleep
   Select a case [1-8]: 1

   CPU0: case 1: active baseline
   CPU0: local=active remote=active window=5 seconds
   CPU0: entering active window
   CPU0: local active window complete
   CPU0 PM counts: runtime-idle=2/2 suspend-to-idle=0/0 standby=0/0

CPU1 reports the matching activity on its own console:

.. code-block:: console

   RT700 dual-core PM sample: CPU1 started on mimxrt700_evk/mimxrt798s/cm33_cpu1
   CPU1: waiting for CPU0 PM test commands
   CPU1: prepared for active window
   CPU1: entering active window
   CPU1: active window complete

Measurement Notes
*****************

Measure RT700 SoC current through JP21 on the EVK. Remove the default JP21
jumper and insert the current meter or power analyzer in series. JP21 measures
the RT700 SoC rail, so it does not isolate CPU0 and CPU1 independently. To see
the contribution of each core, compare the active baseline, single-core sleep,
single-core deep-sleep, and coordinated sleep/deep-sleep/DSR windows.

The default measurement window is 5 seconds. It can be changed at build time
(the value is propagated to both images by sysbuild):

.. code-block:: console

   west build -p always --sysbuild \
     -b mimxrt700_evk/mimxrt798s/cm33_cpu0 \
     samples/boards/nxp/mimxrt700_evk/dual_core_pm \
      -- -DSB_CONFIG_SAMPLE_RT700_DUAL_CORE_PM_SLEEP_SECONDS=30
