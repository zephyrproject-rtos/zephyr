.. zephyr:code-sample:: nxp_mcxw7xx_system_off
   :name: NXP MCXW7x system off
   :relevant-api: sys_poweroff wuc_interface

   Use system off (Power Down or Deep Power Down) on NXP MCXW7x SoCs and wake
   through the WUU wakeup controller.

Overview
********

This sample puts an NXP MCXW70, MCXW71 or MCXW72 SoC into system off. System
off is mapped to Deep Power Down on MCXW71 and MCXW72, and to Power Down on
MCXW70 (Deep Power Down is selectable with
``CONFIG_SOC_MCXW7XX_POWEROFF_DEEP_POWER_DOWN``). In both modes the chip wakes
through the reset routine, so a wakeup looks like a cold boot.

The sample runs ``CONFIG_SAMPLE_APP_TEST_CYCLES`` system-off cycles (default 10) and
then stops, printing a final ``Completed N system-off cycles`` line so a test
harness can confirm every wakeup happened. No console input is needed.

Because each wakeup is a reset, the completed-cycle counter must survive it. The
MCXW7x reset code initialises the ECC SRAM on every reset, the wake reset
included, so the sample keeps the counter in a register file (REGFILE) register,
which keeps its value across a Power Down or Deep Power Down wakeup. On each
boot it tells a wake from a cold boot apart using the reset cause reported
through :ref:`hwinfo <hwinfo_api>` (``RESET_LOW_POWER_WAKE``): a cold boot seeds
the counter, a wake keeps counting. Once the target count is reached the sample
returns without powering off, leaving the SoC awake and debuggable.

The wakeup source is configured through the :ref:`WUC (Wakeup Controller)
<wuc_api>` subsystem (the ``nxp,wuc-wuu`` driver). Two wakeup sources are
selectable at build time:

* ``CONFIG_SAMPLE_SYSTEM_OFF_WAKEUP_TIMER`` (default) arms LPTMR0 through the counter
  alarm API and routes it to the core as a WUU internal-module wakeup source, so
  the board wakes itself after a few seconds with no external action. LPTMR0 is
  the system timer when PM is enabled; without PM the system timer is SysTick,
  so the sample overlay releases LPTMR0 to the counter driver.

* ``CONFIG_SAMPLE_SYSTEM_OFF_WAKEUP_BUTTON`` waits for a press on the board's
  wakeup button (SW2, or SW4 on FRDM-MCXW70). The board devicetree attaches the
  WUU external pin to the button node and aliases it as ``wakeup-button``; the
  sample simply enables ``DT_ALIAS(wakeup_button)``.

Building and Running
********************

Build and flash the sample with the default (timer) wakeup source:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/mcxw7xx/system_off
   :board: frdm_mcxw71
   :goals: build flash
   :compact:

Sample Output
*************

.. code-block:: console

   frdm_mcxw71 system off demo
   Entering system off (cycle 1/10); the timer wakes it in 5 s
   <the timer wakes the SoC after 5 s through the reset routine; main() restarts>
   frdm_mcxw71 system off demo
   Woke from system off; retained count 1/10
   Entering system off (cycle 2/10); the timer wakes it in 5 s
   ...
   frdm_mcxw71 system off demo
   Woke from system off; retained count 10/10
   Completed 10 system-off cycles
