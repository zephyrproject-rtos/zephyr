.. zephyr:code-sample:: stm32_pm_standby
   :name: Standby/Shutdown mode
   :relevant-api: subsys_pm_sys

   Enter and exit Standby mode on STM32.

Overview
********

This sample is a minimum application to demonstrate basic power management of Standby mode
behavior. Press and hold the user button to enter Standby Mode and release the user button
to exit from Standby Mode.

.. _stm32-pm-standby-sample-requirements:

Requirements
************

The board should support enabling PM. For a STM32 based target, it means that
it should support a clock source alternative to Cortex Systick that can be used
in core sleep states, as LPTIM (:dtcompatible:`st,stm32-lptim`).
For another board than nucleo_L476RG please adjust wakeup pin into config_wakeup_features().

Building and Running
********************

Build and flash standby as follows, changing ``nucleo_L476RG`` for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/st/power_mgmt/standby
   :board: nucleo_L476RG
   :goals: build flash
   :compact:

After flashing, the board boots. Press and hold the user button to enter Standby Mode
and release the user button to exit from Standby Mode.

PM configurations
*****************

By default, :kconfig:option:`CONFIG_PM` is enabled.
