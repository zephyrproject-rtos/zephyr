.. zephyr:code-sample:: mimxrt700_evk_system_off
   :name: RT700 System Off
   :relevant-api: sys_poweroff

   Use system off on MIMXRT700-EVK.

Overview
********

This sample demonstrates ``sys_poweroff()`` on the i.MX RT700. It:

* Waits for ENTER on the console, so the run does not start before a current
  measurement is armed.
* Selects the always-on IRTC wake timer as a wakeup source.
* Programs the wake timer to fire ``CONFIG_SAMPLE_RT700_SYSTEM_OFF_WAKEUP_SECONDS``
  (3 by default) seconds in the future.
* Powers the chip off with ``sys_poweroff()``.
* Repeats this three times, counting the cycles it has completed.

By default ``sys_poweroff()`` enters Deep Power Down (DPD): the whole
chip powers off except the RTC / VDD1V8_AON island. The IRTC wake-timer
event is routed through the SLEEPCON WAKEUPEN RTC line, so when the alarm
expires the chip performs a power-on-reset cold boot and the application
restarts.

Only the cold boot stops at the prompt. A DPD wake up is a fresh boot of the
same image, so asking for input again would break the unattended cycling; the
remaining cycles run back to back.

Counting the power-off cycles
*****************************

Because DPD is a cold boot, SRAM cannot hold the cycle counter. The only
state that survives is the VDD1V8_AON island, so the sample parks the
counter in two IRTC alarm registers, which are unused when the wake timer
provides the wake up. ``RTC0->GPR`` looks like a better fit but belongs to
the boot ROM, which stores its flash state context there.

``PMC0->FLAGS[DEEPPDF]`` tells a DPD wake up apart from any other boot, so
a power cycle or a debugger reset restarts the count from zero instead of
picking up a stale value.

Selecting the power-off mode
****************************

``sys_poweroff()`` enters the mode chosen by the SoC Kconfig choice:

* ``CONFIG_SOC_IMXRT7XX_POWEROFF_DPD`` (default) -- Deep Power Down.
* ``CONFIG_SOC_IMXRT7XX_POWEROFF_FDPD`` -- Full Deep Power Down; even
  VDD1V8_PMC is turned off, so wake relies on the always-on RTC path.

SRAM is not retained in either mode; wake is always a cold boot.

Measurement Notes
*****************

Measure RT700 SoC current through JP21 on the EVK. Remove the default JP21
jumper and insert the current meter or power analyzer in series. JP21 carries the
whole SoC rail, which is what makes it the right point for a power-off mode: DPD
and FDPD turn off every domain behind it, so the reading is the residual of the
always-on island rather than a per-core figure.

Arm the meter first, then press ENTER. Each power-off window lasts
``CONFIG_SAMPLE_RT700_SYSTEM_OFF_WAKEUP_SECONDS`` and is bracketed on the console
by ``Powering off`` and the next boot banner, so the three windows show up as
separate plateaus separated by the (much higher) boot and re-arm activity.

Both the window and the ENTER prompt can be changed at build time:

.. code-block:: console

   west build -p always -b mimxrt700_evk/mimxrt798s/cm33_cpu0 \
     samples/boards/nxp/mimxrt700_evk/system_off \
     -- -DCONFIG_SAMPLE_RT700_SYSTEM_OFF_WAKEUP_SECONDS=30 \
     -DCONFIG_SAMPLE_RT700_SYSTEM_OFF_WAIT_FOR_ENTER=n

To compare the two modes, build once per ``CONFIG_SOC_IMXRT7XX_POWEROFF_*``
choice and measure the same window in each. FDPD is the deeper mode on paper, but
it leaves the voltage references in high-power mode so that a rising VDD1V8 can
wake the chip; the SoC clears that overhead only when ``POWERCFG[FDPDBGLP]`` is
set, which the driver does on every entry.

Requirements
************

This application uses the MIMXRT700-EVK.

Building, Flashing and Running
******************************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/mimxrt700_evk/system_off
   :board: mimxrt700_evk/mimxrt798s/cm33_cpu0
   :goals: build flash
   :compact:

Running:

1. Open a UART terminal.
2. Power cycle the device.
3. Press ENTER at the prompt.
4. The device sets the wake-up alarm and powers off (Deep Power Down).
5. After three seconds the RTC wake restarts the application from a cold boot.
6. After three cycles the application stays awake.

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS ***
   Cold boot, powering off 3 times
   Press ENTER to start the power-off cycles
   Wake-up alarm set for 3 seconds
   Powering off
   *** Booting Zephyr OS ***
   Woke up from deep power down, cycle 1 of 3
   Wake-up alarm set for 3 seconds
   Powering off
   *** Booting Zephyr OS ***
   Woke up from deep power down, cycle 2 of 3
   Wake-up alarm set for 3 seconds
   Powering off
   *** Booting Zephyr OS ***
   Woke up from deep power down, cycle 3 of 3
   Done, staying awake
