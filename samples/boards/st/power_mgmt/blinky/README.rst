.. zephyr:code-sample:: stm32_pm_blinky
   :name: Blinky with power management

   Blink an LED using the GPIO API in a low-power context on STM32

Overview
********

This sample is a minimum application to demonstrate basic power management
behavior in a basic blinking LED set up using the :ref:`GPIO API <gpio_api>` in
low power context.
Note that lptim instance selected for the low power timer is named **&stm32_lp_tick_source**
When setting a prescaler to decrease the lptimer input clock frequency, the system can sleep
for a longer  timeout value and the CONFIG_SYS_CLOCK_TICKS_PER_SEC is adjusted.
For example, when clocking the  low power Timer with LSE clock at 32768Hz and adding a
prescaler of <32>, then the kernel sleep period can reach 65536 * 32/32768 = 64 seconds
CONFIG_SYS_CLOCK_TICKS_PER_SEC is set to 1024.

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

After flashing, the LED starts to blink with a fixed period (SLEEP_TIME_MS).
When LPTIM input clock has a prescaler, longer perdiod (up to 64 seconds)
of low power can be tested.


PM configurations
*****************

By default, :kconfig:option:`CONFIG_PM_DEVICE` and :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME` are
enabled, but user can also deactivate one or the other to see each configuration
in play.
