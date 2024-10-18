.. zephyr:code-sample:: stm32_pm_shutdown
   :name: Standby/Shutdown mode
   :relevant-api: sys_poweroff subsys_pm_sys

   Enter and exit Standby/Shutdown mode on STM32.

Overview
********

This sample is a minimum application to demonstrate basic power management of Standby mode and shutdown mode
with WakeUp pins.
User push-button is configured in gpio with interrupt and callback in a basic blinking LED loop.
User can select the low power mode by pressing and holding the push-button:
- when LED2 is OFF, to enter Shutdown Mode
- when LED2 is ON, to enter Standby Mode
The push button is now being configured as WakeUp Source (WakeUp pin)
and system enters in the selected low power mode.
User shall release the push button to exit from standby or from shutdown mode.

.. _stm32-pm-standby_shutdown-sample-requirements:

Requirements
************

The board should support enabling PM. For a STM32 based target, it means that
it should support a clock source alternative to Cortex Systick that can be used
in core sleep states, as LPTIM (:dtcompatible:`st,stm32-lptim`).
For another board than nucleo_L476RG please adjust wakeup pin source through wkup-src alias.
Requires wkup-pin to be defined in the &pwr of the board DTS

Building and Running
********************

Build and flash standby_shutdown as follows, changing ``nucleo_L476RG`` for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/st/power_mgmt/standby_shutdown
   :board: nucleo_l476rg
   :goals: build flash
   :compact:

After flashing, the LED starts to blink.
Press and hold the user button:
when LED2 is OFF to enter to Shutdown Mode
when LED2 is ON to enter to Standby Mode
release the user button to exit from shutdown mode or from shutdown mode.

PM configurations
*****************

By default, :kconfig:option:`CONFIG_PM` is enabled.
