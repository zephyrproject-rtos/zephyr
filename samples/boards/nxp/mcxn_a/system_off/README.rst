.. zephyr:code-sample:: nxp_mcx_system_off
   :name: NXP MCX system off
   :relevant-api: sys_poweroff wuc_interface

   Use system off (Deep Power Down) on NXP MCXA/MCXN SoCs and wake through
   the WUU wakeup controller.

Overview
********

This sample puts an NXP MCXA or MCXN SoC into system off, which is mapped
to the SoC Deep Power Down mode. In Deep Power Down the CORE power domain
is gated and the chip wakes through the reset routine, so a wakeup looks
like a cold boot.

The sample runs ``CONFIG_SAMPLE_APP_TEST_CYCLES`` system-off cycles (default 10) and
then stops, printing a final ``Completed N system-off cycles`` line so a test
harness can confirm every wakeup happened. No console input is needed.

Because each wakeup is a reset, the completed-cycle counter must survive Deep
Power Down. Deep Power Down gates every SRAM array, so the sample keeps the
counter in retained SRAM and arms that SRAM's low-power retention (the VBAT SRAM
LDO on MCXN, the SPC SRAM retention LDO on MCXA) before powering off. On each
boot it tells a wake from a cold boot apart using the reset cause reported
through :ref:`hwinfo <hwinfo_api>` (``RESET_LOW_POWER_WAKE``): a cold boot seeds
the counter, a wake keeps counting. Once the target count is reached the sample
returns without powering off, leaving the SoC awake and debuggable.

The wakeup source is configured through the :ref:`WUC (Wakeup Controller)
<wuc_api>` subsystem (the ``nxp,wuc-wuu`` driver). Two wakeup sources are
selectable at build time:

* ``CONFIG_SAMPLE_SYSTEM_OFF_WAKEUP_TIMER`` (default) arms LPTMR0 through the counter
  alarm API and routes it to the core as a WUU internal-module wakeup source, so
  the board wakes itself after a few seconds with no external action.

* ``CONFIG_SAMPLE_SYSTEM_OFF_WAKEUP_BUTTON`` waits for a transition on a WUU external
  pin. The board devicetree attaches the WUU source to its wakeup button node
  (next to the button ``gpios``) and aliases it as ``wakeup-button``; the sample
  simply enables ``DT_ALIAS(wakeup_button)``. No sample overlay is required. A
  board describes it like::

     &user_button_2 {
         wakeup-ctrls = <&wuu NXP_WUU_PIN_0_FALLING_INT>;
     };

     / {
         aliases {
             wakeup-button = &user_button_2;
         };
     };

Building and Running
********************

Build and flash the sample with the default (timer) wakeup source:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/mcxn_a/system_off
   :board: frdm_mcxn947/mcxn947/cpu0
   :goals: build flash
   :compact:

Sample Output
*************

.. code-block:: console

   frdm_mcxn236 system off demo
   Entering system off (cycle 1/10); the timer wakes it in 5 s
   <the timer wakes the SoC after 5 s through the reset routine; main() restarts>
   frdm_mcxn236 system off demo
   Woke from system off; retained count 1/10
   Entering system off (cycle 2/10); the timer wakes it in 5 s
   ...
   frdm_mcxn236 system off demo
   Woke from system off; retained count 10/10
   Completed 10 system-off cycles

With ``CONFIG_SAMPLE_SYSTEM_OFF_WAKEUP_BUTTON`` each cycle instead waits for a press on
the board's wakeup button (SW2) to leave system off.
