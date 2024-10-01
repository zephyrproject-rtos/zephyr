.. _stm32-pm-stop3:

STM32 PM STOP3 mode
###################

Overview
********

This sample is a minimum application to demonstrate basic power management
behavior in a basic blinking LED set up and STM32U5 STOP3 low power mode.

.. _stm32-pm-stop3-requirements:

Requirements
************

At the moment, only ``nucleo_u575zi_q`` board is supported.
The board shall have an RTC to use it during the standby mode as a replacement
for LPTIM (which is disabled).

Building and Running
********************

Build and flash examples as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/stm32/power_mgmt/stop3
   :board: nucleo_u575zi_q
   :goals: build flash
   :compact:

After flashing, the LED starts to blink.

PM configurations
*****************

By default, :kconfig:option:`CONFIG_PM_DEVICE` and :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME`
are enabled.
