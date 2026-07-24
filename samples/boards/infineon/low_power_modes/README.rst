.. _ifx_low_power_modes:

Infineon Low-Power Modes
########################

Overview
********

This sample exercises the low-power (DeepSleep) modes of an Infineon SoC and
confirms that on-board peripherals recover after each transition.  In DeepSleep
the high-frequency clocks are gated, so peripheral blocks stop; every driver
implements a device ``pm_action`` callback that re-arms its hardware on wake, so
the application does not reconfigure anything itself.

The power modes exercised, from shallowest to deepest, are:

* **Runtime idle** - CPU WFI sleep with the clocks still running.
* **DeepSleep** (suspend-to-idle) - high-frequency clocks gated; SRAM and
  peripheral state retained; wake resumes in-session.
* **DeepSleep-RAM** (suspend-to-ram) - deeper retention with a warm boot on wake;
  drivers are re-initialized from their ``pm_action`` callbacks.
* **DeepSleep-OFF / hibernate** - the terminal power-down modes; wake is a cold
  boot triggered by the wake button, so they end the run.

A set of on-board peripherals (PWM, counter, and, depending on the board,
communication and analog blocks) is self-tested once at startup and again after
each wake.  The exact set is defined by the board devicetree, so it varies per
silicon; a peripheral absent from the devicetree is compiled out and simply
skipped.  A matching result after wake confirms that peripheral's ``pm_action``
restored it.

Application roles
*****************

.. _lpm_roles:

The sample builds one of two images per target, selected in Kconfig:

* **DUT** (``CONFIG_APP_ROLE_DUT``, ``src/main_dut.c``) drives the power-mode
  sequence and the peripheral self-tests.  This is the default role for any
  single-image target.
* **Companion** (``CONFIG_APP_ROLE_COMPANION``, ``src/main_companion.c``) runs on
  the second core of a multi-core SoC and enters the mode the DUT commands over
  an inter-core mailbox.

Two capability options gate the deeper behavior, so a new SoC only enables what
its power management implements:

* ``CONFIG_APP_DEEP_MODES`` - run DeepSleep-RAM and the terminal power-down modes
  in addition to regular DeepSleep.
* ``CONFIG_APP_COMPANION_MBOX`` - command a companion core over the mailbox before
  sleeping.  Single-core SoCs leave this off and drive every register directly.

On the PSOC Edge E84 both options default on with
``CONFIG_PSOC_EDGE_M55_SRF_SUPPORT`` (the TF-M two-application flow) and off in
the sysbuild flow, which runs regular DeepSleep only.

Sequence
********

* **Phase 1 - runtime idle only.** DeepSleep is locked out via the PM policy, so
  the core only enters WFI sleep with clocks running.
* **Phase 2 - deep sleep.** The PM policy is allowed to select DeepSleep, then
  (when supported) DeepSleep-RAM, and finally the terminal power-down mode.  The
  LED thread is suspended so idle periods are long enough for the policy to
  choose the mode; after each in-session wake the sample re-tests every
  peripheral and reports the result.

UART output and deepsleep entry
*******************************

The UART output (printk) in this sample can block the device from entering
DeepSleep.  The sample drains the console FIFO with a helper before each sleep so
that pending output does not influence the sleep behavior.

The UART output generated when the USER button is pressed can block DeepSleep.
So, the USER button can be used to illustrate the blocking behavior.  It may take
several button presses for that behavior to occur.

Supported SoCs
**************

The following SoCs are supported by this sample code:

* infineon/edge/pse84

Building and Running
********************

Make sure to reset the board after programming (XRES) to ensure that the
debugger is not attached, as this prevents the device from entering deep sleep.

Standalone (sysbuild, regular DeepSleep only)
=============================================

This builds the CM55 DUT together with the secure boot stub.  Only regular
DeepSleep is exercised; the companion and the deeper modes are skipped.

.. code-block:: console

   west build -p auto -b kit_pse84_eval/pse846gps2dbzc4a/m55 \
     samples/boards/infineon/low_power_modes --sysbuild
   west flash

TF-M + mcuboot multi-core flow (all modes)
==========================================

The deeper modes need the CM33 non-secure (TF-M) companion, which arms the secure
side and votes the SoC into DeepSleep.  Build and flash three images in order: the
mcuboot bootloader, the CM33-NS companion, then the CM55 DUT.  The DUT build points
at the companion build directory with ``-DPSE84_CM33_BUILD_DIR`` so the two images
are chain-loaded together.

Build and flash the mcuboot bootloader:

.. code-block:: console

   west build -p auto -b kit_pse84_eval/pse846gps2dbzc4a/m33 \
     ../bootloader/mcuboot/boot/zephyr -d build_mcuboot -- \
     -DEXTRA_DTC_OVERLAY_FILE="$PWD/boards/infineon/kit_pse84_eval/kit_pse84_eval_mcuboot_bl.overlay" \
     -DEXTRA_CONF_FILE="$PWD/boards/infineon/kit_pse84_eval/kit_pse84_eval_mcuboot.conf" \
     -DCONFIG_UPDATEABLE_IMAGE_NUMBER=3
   west flash -d build_mcuboot

Build and flash the CM33 non-secure companion:

.. code-block:: console

   west build -p auto -b kit_pse84_eval/pse846gps2dbzc4a/m33/ns \
     samples/boards/infineon/low_power_modes -d build_tfm_ns -- \
     -DEXTRA_CONF_FILE="$PWD/boards/infineon/kit_pse84_eval/kit_pse84_eval_slot.conf" \
     -DCONFIG_PSOC_EDGE_M55_SRF_SUPPORT=y
   west flash -d build_tfm_ns

Build and flash the CM55 DUT, pointing it at the CM33-NS build:

.. code-block:: console

   west build -p auto -b kit_pse84_eval/pse846gps2dbzc4a/m55 \
     samples/boards/infineon/low_power_modes -d build_multicore_55 -- \
     -DCONFIG_PSOC_EDGE_M55_SRF_SUPPORT=y \
     -DPSE84_CM33_BUILD_DIR=build_tfm_ns \
     -DEXTRA_CONF_FILE="$PWD/boards/infineon/kit_pse84_eval/kit_pse84_eval_slot.conf"
   west flash -d build_multicore_55

Sample Output
=============

To check output of this sample, open a serial console, and select the proper COM port
(i.e. on Linux minicom, putty, screen, etc).

The console output should look approximately like this:

.. code-block:: console

   *** Booting Zephyr OS build ***
   PWM blue LED started at 2 Hz
   Free-running counter started
   Phase 1: runtime-idle only
   Sleep: 5 | Deepsleep: 0
   Phase 2: run each low-power mode in sequence
   Phase 2: exercising DeepSleep
   Phase 2: enter DeepSleep (counter=12345, watch blue LED freeze)
   Phase 2: woke after 2000 ms
   Sleep: 5 | Deepsleep: 1
   Phase 2: exercising DeepSleep-RAM
   Phase 2: enter DeepSleep-RAM (counter=23456, watch blue LED freeze)
   Phase 2: woke after 2000 ms
   Sleep: 5 | Deepsleep: 1
   Phase 2: exercising DeepSleep-OFF
   Phase 2: entering DeepSleep-OFF. Press the button to wake/reset.
   Sequence complete

The per-mode peripheral self-test lines between the wake messages depend on the
board devicetree and are omitted here.  On a single-core target, or in the
sysbuild flow, the DeepSleep-RAM and terminal power-down lines are replaced by a
single ``regular DeepSleep only`` notice.
