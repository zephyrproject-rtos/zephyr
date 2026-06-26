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

Configuration
*************

The sample builds a single device-under-test image (``src/main_dut.c``) that
drives the power-mode sequence and the peripheral self-tests.  One capability
option gates the deeper behavior:

* ``CONFIG_APP_DEEP_MODES`` - run DeepSleep-RAM and the terminal power-down modes
  (DeepSleep-OFF / hibernate) in addition to regular DeepSleep.  Leave it
  disabled on a target whose power management only implements regular DeepSleep.

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
DeepSleep.  ``printk`` is synchronous and the console UART driver refuses to
suspend while a transmission is still in flight, so pending output reaches the
wire before the clocks are gated without any application-level draining.

The UART output generated when the USER button is pressed can block DeepSleep.
So, the USER button can be used to illustrate the blocking behavior.  It may take
several button presses for that behavior to occur.

Supported SoCs
**************

The following SoCs are supported by this sample code:

* infineon/cat1b/psc3

Building and Running
********************

Make sure to reset the board after programming (XRES) to ensure that the
debugger is not attached, as this prevents the device from entering deep sleep.

.. code-block:: console

   west build -p auto -b kit_psc3m5_evk/psc3m5fds2afq1 \
     samples/boards/infineon/low_power_modes
   west flash

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
board devicetree and are omitted here.  When ``CONFIG_APP_DEEP_MODES`` is
disabled, the DeepSleep-RAM and terminal power-down lines are replaced by a
single ``regular DeepSleep only`` notice.
