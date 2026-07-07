.. zephyr:code-sample:: mimxrt700_evk_system_off
   :name: RT700 System Off
   :relevant-api: sys_poweroff

   Use system off on MIMXRT700-EVK.

Overview
********

This sample demonstrates ``sys_poweroff()`` on the i.MX RT700. It:

* Selects the always-on IRTC wake timer as a wakeup source.
* Programs the wake timer to fire 10 seconds in the future.
* Powers the chip off with ``sys_poweroff()``.

By default ``sys_poweroff()`` enters Deep Power Down (DPD): the whole
chip powers off except the RTC / VDD1V8_AON island. The IRTC wake-timer
event is routed through the SLEEPCON WAKEUPEN RTC line, so when the alarm
expires the chip performs a power-on-reset cold boot and the application
restarts.

Selecting the power-off mode
****************************

``sys_poweroff()`` enters the mode chosen by the SoC Kconfig choice:

* ``CONFIG_SOC_IMXRT7XX_POWEROFF_DPD`` (default) -- Deep Power Down.
* ``CONFIG_SOC_IMXRT7XX_POWEROFF_FDPD`` -- Full Deep Power Down; even
VDD1V8_PMC is turned off, so wake relies on the always-on RTC path.

SRAM is not retained in either mode; wake is always a cold boot.

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
3. The device sets the wake-up alarm and powers off (Deep Power Down).
4. After 10 seconds the RTC wake restarts the application from a cold boot.

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS ***
   Wake-up alarm set for 10 seconds
   Powering off
