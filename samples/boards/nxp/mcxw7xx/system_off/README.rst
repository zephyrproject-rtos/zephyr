.. zephyr:code-sample:: nxp_mcxw7xx_system_off
   :name: NXP MCXW7x system off
   :relevant-api: sys_poweroff wuc_interface

   Use system off (Power Down or Deep Power Down) on NXP MCXW7x SoCs and wake
   through the WUU wakeup controller.

Overview
********

This sample puts an NXP MCXW70, MCXW71 or MCXW72 SoC into system off. On MCXW70,
system off defaults to Power Down; Deep Power Down can be selected with
``CONFIG_SOC_MCXW7XX_POWEROFF_DEEP_POWER_DOWN``. On MCXW71 and MCXW72, system
off always uses Deep Power Down 2 (DPD2), without application SRAM retention.
Power Down is not a system-off option on these two SoCs.

All supported modes wake through the normal boot path: the kernel restarts
and ``main()`` runs again, rather than resuming the previous execution context.
MCXW71 and MCXW72 support ROM-assisted resume, but this system-off
implementation does not use it.

On MCXW71 and MCXW72, the system-off hook disables the backup SRAM regulator
before entering DPD2, even if the application previously enabled it. The
application must not lock that regulator enabled: if it cannot be disabled,
the hook reports a fatal error instead of entering a retention-enabled mode.
Other VBAT clock and wakeup settings are left unchanged.
SRAM supply switching must remain in automatic mode; software-forced VBAT
supply is not supported by this hook.

The sample runs ``CONFIG_SAMPLE_APP_TEST_CYCLES`` system-off cycles (default 10) and
then stops, printing a final ``Completed N system-off cycles`` line so a test
harness can confirm every wakeup happened. No console input is needed.

The cycle counter must survive each system-off wakeup. The current normal
startup path initializes SRAM, and DPD2 on MCXW71 and MCXW72 also disables
SRAM retention. The sample uses a register file (REGFILE) register on all
three SoCs instead. The register retains its value across Power Down and
Deep Power Down wakeup without retaining the application's execution context.

On each boot the sample checks ``RESET_LOW_POWER_WAKE`` through
:ref:`hwinfo <hwinfo_api>`. A reset without this flag resets the counter; a
low-power wakeup continues counting. Once the target count is reached the
sample returns without powering off, leaving the SoC awake and debuggable.

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
  sample enables it through the WUC API directly because the GPIO driver does
  not configure WUC wakeup sources.

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
