.. zephyr:code-sample:: pm_sleep
   :name: Unified system sleep
   :relevant-api: subsys_pm_sys

   Enter light and deep low-power states portably with
   :c:func:`pm_light_sleep` and :c:func:`pm_deep_sleep`.

Overview
********

This sample demonstrates the portable, hardware-independent system sleep
helpers :c:func:`pm_light_sleep` and :c:func:`pm_deep_sleep`. These helpers pick
the most appropriate power state declared for the CPU in devicetree and enter it
through the standard power-management subsystem, so the *same application code*
runs unchanged across vendors (Espressif, NXP, STM32, Nordic, ...).

* :c:func:`pm_light_sleep` enters the **shallowest** low-power state the board
  declares (fastest wake-up). Context is retained.
* :c:func:`pm_deep_sleep` enters the **deepest context-retaining** state the
  board declares (up to :c:enumerator:`PM_STATE_SUSPEND_TO_DISK`) and resumes in
  place. :c:enumerator:`PM_STATE_SOFT_OFF` is never selected.
* :c:func:`pm_soft_off` enters :c:enumerator:`PM_STATE_SOFT_OFF` (no context
  retention); the system resets on wake-up and the call does not return.

How it maps to hardware
***********************

The helpers contain **no vendor code**. Each SoC already implements
:c:func:`pm_state_set`, which translates the generic Zephyr power states into
silicon-specific operations. The *same application code* therefore runs
unchanged on any board; the only thing that differs per board is which power
states it declares in devicetree.

What ``pm_light_sleep()`` / ``pm_deep_sleep()`` resolve to on the boards this
sample is tested on:

* ``native_sim`` --- ``suspend-to-idle`` / ``suspend-to-ram``
* ``esp32_devkitc`` --- ``standby`` / ``standby`` (deep-sleep ``soft-off`` via
  ``pm_soft_off()``)
* ``esp32s3_devkitc`` --- ``standby`` / ``standby`` (deep-sleep ``soft-off`` via
  ``pm_soft_off()``)
* ``frdm_mcxa153`` --- ``runtime-idle`` / ``standby`` (powerdown)
* ``frdm_mcxn236`` --- ``runtime-idle`` / ``standby`` (powerdown)

If the board declares no low-power state, the helpers return ``-ENOTSUP``
instead of doing something surprising, so the application can react accordingly.

Requirements
************

Any board that declares one or more power states in devicetree and enables
:kconfig:option:`CONFIG_PM`. No board-specific application code is required.

This sample is set up to run on :ref:`native_sim <native_sim>`, where it provides
a small *simulated* PM backend (enabled by ``CONFIG_APP_SIM_PM_BACKEND``) and a
devicetree overlay declaring a ``suspend-to-idle`` and a ``suspend-to-ram``
state. On real hardware the SoC supplies the backend and the board's own power
states are used, so ``CONFIG_APP_SIM_PM_BACKEND`` stays disabled.

On Espressif targets (``esp32_devkitc``, ``esp32s3_devkitc``), light sleep wakes
from the RTC timer, so the sample enables :kconfig:option:`CONFIG_PM_DEVICE` and
:kconfig:option:`CONFIG_COUNTER` and marks ``&rtc_timer`` as a wake-up source in
the matching ``boards/<board>_procpu.conf`` and ``.overlay`` files.

On the NXP MCX boards (``frdm_mcxa153``, ``frdm_mcxn236``) the low-power modes
gate the main clocks, so the sample enables the LPTMR as the wake-up timer via
``boards/frdm_mcxa153.{conf,overlay}`` and ``boards/frdm_mcxn236.{conf,overlay}``.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/pm/pm_sleep
   :host-os: unix
   :board: native_sim
   :goals: build run
   :compact:

To build for real hardware, target any board that declares power states; the
SoC provides the backend automatically. For example::

   west build -b esp32_devkitc/esp32/procpu samples/subsys/pm/pm_sleep
   west build -b frdm_mcxa153/mcxa153        samples/subsys/pm/pm_sleep
   west build -b frdm_mcxn236/mcxn236        samples/subsys/pm/pm_sleep

Using the API in your own application
*************************************

.. code-block:: c

   #include <zephyr/pm/pm.h>

   /* Light sleep for up to 500 ms (shallowest state). */
   pm_light_sleep(K_MSEC(500));

   /* Deep sleep for up to 1 second (deepest context-retaining state). */
   pm_deep_sleep(K_SECONDS(1));

   /* Soft-off for up to 1 second (no context retention; resets on wake). */
   pm_soft_off(K_SECONDS(1));

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build zephyr-vX.Y.Z ***
   PM unified sleep sample
   Requesting light sleep for 500 ms...
     sim: SoC entering suspend-to-idle
   Resumed from light sleep
   Requesting deep sleep for 1 s...
     sim: SoC entering suspend-to-ram
   Resumed from deep sleep
   Requesting soft-off (context not preserved)...
     sim: SoC entering soft-off
   Resumed from soft-off (simulated)
   PM unified sleep sample complete
