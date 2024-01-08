.. _stm32-pm-adc-sample:

STM32 PM ADC
############

Overview
********

This sample is a minimum application to demonstrate basic power management
behavior in a basic ADC set up in low power context.

.. _stm32-pm-adc-sample-requirements:

Requirements
************

The board should support enabling PM. For a STM32 based target, it means that
it should support a clock source alternative to Cortex Systick that can be used
in core sleep states, as LPTIM (:dtcompatible:`st,stm32-lptim`).

Building and Running
********************

Build and flash as follows, changing ``nucleo_wb55rg`` for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/stm32/power_mgmt/adc
   :board: nucleo_wb55rg
   :goals: build flash
   :compact:

After flashing, the console shows the ADC measurement in the form:
``ADC reading[0]:``
``- adc@50040000, channel 3: 1158 = 932 mV``

PM configurations
*****************

By default, :kconfig:option:`CONFIG_PM_DEVICE` and :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME` are
enabled.
On STM32WB, we can observe a power consumption of about 25µA with both kconfig
enabled, 27.5µA without (each time with :kconfig:option:`CONFIG_PM` enabled).
