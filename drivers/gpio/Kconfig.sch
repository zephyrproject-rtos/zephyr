# Kconfig.sch - Intel SCH GPIO configuration options
#
#
# Copyright (c) 2016 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#

menuconfig GPIO_SCH
	bool "Intel SCH GPIO controller"
	depends on GPIO
	default n
	help
	  Enable the SCH GPIO driver found on Intel boards

if GPIO_SCH

config GPIO_SCH_INIT_PRIORITY
	int
	depends on GPIO_SCH
	default 60
	prompt "Init priority"
	help
	  Device driver initialization priority.

config GPIO_SCH_0
	bool "Enable SCH GPIO port 0"
	default 0
	depends on GPIO_SCH

config GPIO_SCH_0_DEV_NAME
	string "Name of the device"
	depends on GPIO_SCH_0
	default "GPIO_0"

config GPIO_SCH_1
	bool "Enable SCH GPIO port 1"
	default n
	depends on GPIO_SCH

config GPIO_SCH_1_DEV_NAME
	string "Name of the device"
	depends on GPIO_SCH_1
	default "GPIO_1"

endif # GPIO_SCH
