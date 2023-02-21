.. _stm32-pm-blinky-sample:

STM32 PM Blinky
###############

Overview
********

This sample is a minimum application to demonstrate basic power management of Standby mode and
shutdown mode
behavior in a basic blinking LED set up you can enter in shutdown mode or in standbymode mode.
press and hold the user button to enter to shutdown mode when LED2(GREEN) is ON.
press and hold the user button to enter to standby mode when LED2(GREEN) is OFF.
release the user button to exit from shutdown mode or from shutdown mode.

.. _stm32-pm-blinky_standby_shutdown_mode-sample-requirements:

Requirements
************

The board should support enabling PM. For a STM32 based target, it means that
it should support a clock source alternative to Cortex Systick that can be used
in core sleep states, as LPTIM (:dtcompatible:`st,stm32-lptim`).

Building and Running
********************

Build and flash blinky_standby_shutdown_mode as follows, changing ``nucleo_L476RG`` for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/samples/boards/stm32/power_mgmt/blinky_standby_shutdown_mode
   :board: nucleo_L476RG
   :goals: build flash
   :compact:

After flashing, the LED starts to blink.
press and hold the user button to enter to shutdown mode when led2 is ON.
press and hold the user button to enter to standby mode when led2 is OFF.
release the user button to exit from shutdown mode or from shutdown mode.



PM configurations
*****************

By default, :kconfig:option:`CONFIG_PM_DEVICE` is enabled.
