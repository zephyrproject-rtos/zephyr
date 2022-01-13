.. _stm32-pm-blinky-rtc-wakeup-sample:

STM32 PM Blinky RTC Wakeup
##########################

Overview
********

This sample shows the wakeup of a soft-off state via a RTC.

.. _stm32-pm-blinky-rtc-wakeup-sample-requirements:

Requirements
************

The board should support enabling PM and a RTC. For a STM32 based target,
it means that it should support a clock source alternative to Cortex Systick
that can be used in core sleep states, as LPTIM (:ref:`dtbinding_st_stm32_lptim`).

Building and Running
********************

Build and flash Blinky RTC wakeup as follows,
changing ``stm32h735g_disco`` for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/stm32/power_mgmt/blinky_rt_wakeup
   :board: stm32h735g_disco
   :goals: build flash
   :compact:

After flashing, the LED starts to blink. In between the CPU will restart, as a
wakeup from standby is almost the same as a CPU reset.

PM configurations
*****************

By default, :kconfig:`CONFIG_PM_DEVICE` and :kconfig:`CONFIG_PM_DEVICE_RUNTIME` are
enabled, but user can also deactivate one or the other to see each configuration
in play.
