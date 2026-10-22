.. zephyr:code-sample:: adi-gpio-wakeup
   :name: ADI MAX32 GPIO Wakeup

   Use GPIO to wake the system from low-power states

Overview
********

This sample demonstrates using GPIO to wake up the system from low-power states on MAX32 SoCs.
It configures a button as a wakeup source and then enters each of the supported PM states
in turn. A long button press powers off the system. The user can verify that the system
can wake up from each state by pressing the button.

If powered off, the device can only wake up using the reset button, a power cycle, or a dedicated
wakeup input. See ``wk_pin`` in the board's devicetree for the dedicated wakeup input. If this
pin is not connected to a button, it needs to be manually shorted to ground to wake up the system
from the power-off state.

A power profiling tool can be used to verify that the system is entering the expected low-power
states and consuming lower power while in those states.

.. _max32-pm-gpio_wakeup-sample-requirements:

Requirements
************

The board should support power management and have a GPIO that can be used as a wakeup source.
The button used in this sample is defined by the ``sw0`` alias in the board's devicetree,
so the board should have that alias defined and pointing to a GPIO pin connected to a button.

Building and Running
********************

Build and flash gpio_wakeup as follows, changing ``max32655evkit`` for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/adi/power_mgmt/gpio_wakeup
   :board: max32655evkit/max32655/m4
   :goals: build flash
   :compact:

PM configurations
*****************

By default, :kconfig:option:`CONFIG_PM`, :kconfig:option:`CONFIG_PM_POLICY_DEVICE_CONSTRAINTS`,
:kconfig:option:`CONFIG_PM_DEVICE` and :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME` are enabled.

Deferred logging is disabled in this sample to ensure that the system does not schedule
deferred work which could wake the system up while it is in a low-power state.
