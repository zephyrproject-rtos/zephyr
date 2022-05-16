.. _boards-stm32-power_mgmt-stm32wb_ble-sample:

STM32: PM BLE Power Management (STM32WB)
########################################

Overview
********

A simple application demonstrating the BLE operations (advertising) with
Zephyr power management enabled (:kconfig:option:`CONFIG_PM`).

After startup, a first 2 seconds beacon is performed, 1 second break and
beacon is started again.
BLE link is then disabled and started up again after 2 seconds, then same
beacon sequence happens.

Finally, platform shutdown is triggered. It can be restarted by pressing the
board reset button.

Using a power measurement tool, user can observe the platform reaching STOP2 mode
before beacon is started and between advertising peaks besides as SHUTDOWN mode
when requested.

Requirements
************

* For now, only compatible with nucleo_wb55rg.
