.. zephyr:code-sample:: stm32_pm_suspend_to_ram
   :name: Suspend to RAM
   :relevant-api: subsys_pm_device_runtime

   Use suspend to RAM low power mode on STM32.

Overview
********

This sample is a minimum application to demonstrate basic power management
behavior in a basic blinking LED set up using the :ref:`GPIO API <gpio_api>` in
low power context + ADC measurements and entropy.
SPI loopback is also available but not yet implemented for Suspend To RAM PM
mode.

.. _stm32-pm-suspend-to-ram-sample-requirements:

Requirements
************

The board should support enabling PM. For a STM32 based target, it means that
it should support a clock source alternative to Cortex Systick that can be used
in core sleep states, as LPTIM (:dtcompatible:`st,stm32-lptim`).
The board shall have an RTC to use it during the standby mode as a replacement
for LPTIM (which is disabled). The board shall also have RAM retention to be
able to restore context after standby.

Building and Running
********************

Build and flash Blinky as follows, changing ``stm32wba55cg`` for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/st/power_mgmt/suspend_to_ram
   :board: stm32wba55cg
   :goals: build flash
   :compact:

After flashing, the LED starts to blink.

PM configurations
*****************

By default, :kconfig:option:`CONFIG_PM_DEVICE` and :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME`
are enabled.
