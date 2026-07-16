.. zephyr:code-sample:: nxp_mcx_s2ram
   :name: NXP MCX suspend-to-RAM
   :relevant-api: subsys_pm_states wuc_interface

   Suspend an NXP MCXA/MCXN SoC to RAM and resume on an LPTMR timer (or button)
   wakeup delivered through the WUU wakeup controller.

Overview
********

This sample exercises :c:enumerator:`PM_STATE_SUSPEND_TO_RAM` on NXP MCXA and
MCXN SoCs, where it is mapped to the SoC Deep Power Down mode. The CORE power
domain is gated and the chip wakes through the reset routine; the
:ref:`pm_s2ram <pm-guide>` resume path then returns directly to the suspend call
site without re-running kernel or CPU initialization, so on both families the
resume is transparent (resume-in-place) to the application.

A stand-in application thread models a real workload: it does its work and then
blocks waiting for its next event, with no knowledge of Deep Power Down. The
thread suspends the SoC to RAM and the wakeup resumes it transparently, right
where it blocked. A counter incremented before every suspend survives the cycle,
so the printed value demonstrates the retained RAM.

The thread repeats for ``CONFIG_SAMPLE_APP_TEST_CYCLES`` cycles (default 10) and then
stops, printing a final ``Completed N suspend-to-RAM cycles`` line so a test
harness can confirm every wakeup happened. No console input is needed. Once the
run is over the SoC stays awake (every PM state is locked, so it only ever
suspends when the sample explicitly asks it to).

The wakeup source is configured through the :ref:`WUC (Wakeup Controller)
<wuc_api>` subsystem and is selectable at build time:

* ``CONFIG_SAMPLE_S2RAM_WAKEUP_TIMER`` (default) arms LPTMR0 through the counter alarm
  API and routes it to the core as a WUU internal-module wakeup source. The
  worker thread blocks in :c:func:`k_sleep`, so the cycle repeats automatically.

* ``CONFIG_SAMPLE_S2RAM_WAKEUP_BUTTON`` resumes on a WUU external pin transition (the
  board's ``wakeup-button``, SW2). Deep Power Down resets the GPIO, so there is
  no live GPIO edge to catch on resume - the press is latched only by the WUU.
  The sample therefore treats the suspend-to-RAM resume itself as the wake signal
  (no other wakeup source is armed), observed from a PM notifier that releases the
  worker thread. Press SW2 to advance each cycle.

.. note::

   On MCXN SoCs only SRAMA (the first 32 KB, retained by the VBAT RAM LDO)
   survives Deep Power Down, so the whole retained working set must fit in SRAMA
   and ``zephyr,sram`` must point to it. On MCXA SoCs all SRAM is retained by the
   SPC SRAM retention LDO, so no placement constraint applies.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/mcxn_a/s2ram
   :board: frdm_mcxa156
   :goals: build flash
   :compact:

Sample Output
*************

.. code-block:: console

   frdm_mcxa156 suspend-to-RAM demo
   Retained S2RAM cycle counter: 0
   Entering suspend-to-RAM (cycle 1/10); wake in 3 s
   Resumed from suspend-to-RAM; retained counter is 1
   Entering suspend-to-RAM (cycle 2/10); wake in 3 s
   Resumed from suspend-to-RAM; retained counter is 2
   ...
   Entering suspend-to-RAM (cycle 10/10); wake in 3 s
   Resumed from suspend-to-RAM; retained counter is 10
   Completed 10 suspend-to-RAM cycles

With ``CONFIG_SAMPLE_S2RAM_WAKEUP_BUTTON`` each cycle instead waits for an SW2 press:

.. code-block:: console

   frdm_mcxn236 suspend-to-RAM demo
   Retained S2RAM cycle counter: 0
   Entering suspend-to-RAM (cycle 1/10); press SW2 to wake
   Resumed from suspend-to-RAM; retained counter is 1
   Entering suspend-to-RAM (cycle 2/10); press SW2 to wake
   Resumed from suspend-to-RAM; retained counter is 2
   ...
