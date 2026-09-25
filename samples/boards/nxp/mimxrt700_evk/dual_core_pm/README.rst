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
that counts state entry/exit events. CPU0 is the test orchestrator: it waits for
ENTER on its console, then walks the whole test matrix once and stops. For each
case it hands CPU1 a mode over the MU (Messaging Unit) mailbox. The two cores
then enter their windows together through a small handshake:

#. CPU0 sends the mode command, CPU1 acknowledges that it is ready;
#. CPU0 sends ``GO`` to release CPU1;
#. each core runs its own measurement window for the configured number of
   seconds;
#. CPU1 reports that its window is complete.

Consecutive cases are separated by ``CONFIG_SAMPLE_RT700_DUAL_CORE_PM_SETTLE_SECONDS``
of quiet, so they show up as separate plateaus on a current trace. Once the last
case is done CPU0 prints a summary line and stays idle; press the reset button to
run the matrix again.

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

.. note::

   Case 8 relies on device power management for the console. DSR collapses
   VDD2_COMP, which takes the LP_FLEXCOMM/LPUART console with it: the wrapper's
   ``PSELID[PERSEL]`` selection and the whole LPUART register bank are lost, so a
   console write after the window would stall on a transmitter that is no longer
   clocked, or bus-fault on the block. ``CONFIG_PM_DEVICE_SYSTEM_MANAGED`` brackets
   the window with ``pm_suspend_devices()`` / ``pm_resume_devices()``, and the SoC's
   ``peripheral-domain`` node reports ``PM_STATE_STANDBY`` as a real power cycle to
   everything tagged with it, so the two drivers re-run
   ``PM_DEVICE_ACTION_TURN_ON`` and rebuild their hardware from scratch. The single
   NUL byte seen on the console as the window opens is a pad-power artifact, not
   lost output.

The sample does not configure any PMC register masks, and neither image keeps a
shared resource alive on the other core's behalf. Every resource both cores can
vote on is aggregated by the PMC: a resource only powers down when both domains
agree, so each core votes to keep exactly what it uses itself and votes the rest
down. CPU1 casts its own claims -- the SENSE/COMN main clocks, the RAM arbiter
clock, VDDN_COM and its code and data partitions -- from the board's early init
hook as it boots, which is what lets CPU1 keep running while CPU0 deep-sleeps in
cases 5-7. All the application does is coordinate over the MU; the ``pm_policy``
locks above only select which state a window exercises, and nothing holds CPU0
out of deep sleep while CPU1 runs.

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

After reset CPU0 lists the cases and waits for ENTER, which is the only console
input the sample needs: press it once the measurement equipment is armed and case
1 through case 8 then run back to back without further interaction. On the CPU0
console:

.. code-block:: console

   RT700 dual-core PM sample: CPU0 started on mimxrt700_evk/mimxrt798s/cm33_cpu0

   RT700 dual-core PM cases:
     1: active baseline
     2: CPU0 sleep, CPU1 active
     3: CPU0 active, CPU1 sleep
     4: CPU0 and CPU1 sleep
     5: CPU0 deep sleep, CPU1 active
     6: CPU0 active, CPU1 deep sleep
     7: CPU0 and CPU1 deep sleep
     8: CPU0 deep sleep retention, CPU1 deep sleep

   Press ENTER to run all 8 cases once:

   CPU0: run: 8 cases, window=2 s, settle=2 s

   CPU0: case 1: active baseline
   CPU0: local=active remote=active window=2 seconds
   CPU0: entering active window
   CPU0: local active window complete
   CPU0 PM counts: runtime-idle=2/2 suspend-to-idle=0/0 standby=0/0

   ... cases 2 to 8 ...

   CPU0: run complete: 8 cases

CPU1 reports the matching activity on its own console:

.. code-block:: console

   RT700 dual-core PM sample: CPU1 started on mimxrt700_evk/mimxrt798s/cm33_cpu1
   CPU1: waiting for CPU0 PM test commands
   CPU1: prepared for active window
   CPU1: entering active window
   CPU1: active window complete

.. note::

   Between two cases CPU1 spins on the MU mailbox with ``k_yield()`` instead of
   sleeping, so that waiting never adds PM state entries of its own and the
   per-case counters stay readable. The settle gap is therefore not a valid
   low-power measurement window on CPU1; measure inside the windows the console
   brackets with ``entering``/``complete``.

Measurement Notes
*****************

Measure RT700 SoC current through JP21 on the EVK. Remove the default JP21
jumper and insert the current meter or power analyzer in series. JP21 measures
the RT700 SoC rail, so it does not isolate CPU0 and CPU1 independently. To see
the contribution of each core, compare the active baseline, single-core sleep,
single-core deep-sleep, and coordinated sleep/deep-sleep/DSR windows.

The default measurement window is 2 seconds, followed by a 2 second settle gap.
Both can be changed at build time (sysbuild propagates the window to both images
and the settle time to CPU0):

.. code-block:: console

   west build -p always --sysbuild \
     -b mimxrt700_evk/mimxrt798s/cm33_cpu0 \
     samples/boards/nxp/mimxrt700_evk/dual_core_pm \
      -- -DSB_CONFIG_SAMPLE_RT700_DUAL_CORE_PM_SLEEP_SECONDS=30 \
      -DSB_CONFIG_SAMPLE_RT700_DUAL_CORE_PM_SETTLE_SECONDS=5
