.. _stm32-pm-blinky-sample:

STM32 PM Blinky
###############

Overview
********

This sample is a minimum application to demonstrate basic power management
behavior in a basic blinking LED set up using the :ref:`GPIO API <gpio_api>` in
low power context.

.. _stm32-pm-blinky-sample-requirements:

Requirements
************

The board should support enabling PM. For a STM32 based target, it means that
it should support a clock source alternative to Cortex Systick that can be used
in core sleep states, as LPTIM (:dtcompatible:`st,stm32-lptim`).

Building and Running
********************

Build and flash Blinky as follows, changing ``stm32l562e_dk`` for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: stm32l562e_dk
   :goals: build flash
   :compact:

After flashing, the LED starts to blink.

PM configurations
*****************

By default, :kconfig:option:`CONFIG_PM_DEVICE` and :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME` are
enabled, but user can also deactivate one or the other to see each configuration
in play.
