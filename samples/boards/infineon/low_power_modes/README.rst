.. _ifx_low_power_modes:

Infineon Low-Power Modes
########################

Overview
********

This sample exercises the low-power (DeepSleep) modes of an Infineon SoC.  In
DeepSleep the high-frequency clocks are gated, so peripheral blocks stop; every
driver implements a device ``pm_action`` callback that re-arms its hardware on
wake, so the application does not reconfigure anything itself.

The power modes exercised, from shallowest to deepest, are:

* **Runtime idle** - CPU WFI sleep with the clocks still running.
* **DeepSleep** (suspend-to-idle) - high-frequency clocks gated; SRAM and
  peripheral state retained; wake resumes in-session.
* **DeepSleep-RAM** (suspend-to-ram) - deeper retention with a warm boot on wake;
  drivers are re-initialized from their ``pm_action`` callbacks.
* **DeepSleep-OFF / hibernate** - the terminal power-down modes; wake is a cold
  boot triggered by the wake button, so they end the run.

Configuration
*************

The sample builds a single device-under-test image (``src/main_dut.c``) that
drives the power-mode sequence.  One capability option gates the deeper
behavior:

* ``CONFIG_APP_DEEP_MODES`` - run DeepSleep-RAM and the terminal power-down modes
  (DeepSleep-OFF / hibernate) in addition to regular DeepSleep.  Leave it
  disabled on a target whose power management only implements regular DeepSleep.

Sequence
********

* **Phase 1 - runtime idle only.** DeepSleep is locked out via the PM policy, so
  the core only enters WFI sleep with clocks running.
* **Phase 2 - deep sleep.** The PM policy is allowed to select DeepSleep, then
  (when supported) DeepSleep-RAM, and finally the terminal power-down mode.
  Each idle window is long enough for the policy to choose the mode.

UART output and deepsleep entry
*******************************

The UART output (printk) in this sample can block the device from entering
DeepSleep. ``printk`` is synchronous and the console UART driver refuses to
suspend while a transmission is in flight, so pending output reaches the wire
before the clocks are gated. The output generated on a USER button press can be
used to illustrate this; it may take several presses to observe.

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
   Phase 1: runtime-idle only
   Sleep: 1 | Deepsleep: 0 | Deepsleep-RAM: 0
   Phase 2: run each low-power mode in sequence
   Phase 2: exercising DeepSleep
   Phase 2: woke after 2000 ms
   Sleep: 1 | Deepsleep: 1 | Deepsleep-RAM: 0
   Phase 2: exercising DeepSleep-RAM
   Phase 2: woke after 2000 ms
   Sleep: 3 | Deepsleep: 1 | Deepsleep-RAM: 1
   Phase 2: press the button to toggle the terminal mode (entered after 2000 ms idle)
   Phase 2: terminal mode = DeepSleep-OFF
   Phase 2: entering DeepSleep-OFF. Press the button to wake/reset.

DeepSleep-RAM resumes in-session through the SoC's suspend-to-RAM path, so the
boot banner is not printed again; the ``Deepsleep-RAM`` count advancing is what
shows the state engaged.
