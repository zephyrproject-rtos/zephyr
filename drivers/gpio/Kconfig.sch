# Kconfig.sch - Intel SCH GPIO configuration options
#
#
# Copyright (c) 2016 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

menuconfig GPIO_SCH
	bool "Intel SCH GPIO controller"
	depends on GPIO
	default n
	help
	  Enable the SCH GPIO driver found on Intel platforms

if GPIO_SCH

config GPIO_SCH_INIT_PRIORITY
	int
	depends on GPIO_SCH
	default 60
	prompt "Init priority"
	help
	  Device driver initialization priority.

config GPIO_SCH_LEGACY_IO_PORTS_ACCESS
	bool "SCH registers accessed through legacy I/O ports"
	depends on GPIO_SCH
	default n
	help
	  Enable the legacy I/O ports access methods for SCH registers.

config GPIO_SCH_0
	bool "Enable SCH GPIO port 0"
	default 0
	depends on GPIO_SCH

config GPIO_SCH_0_DEV_NAME
	string "Name of the device"
	depends on GPIO_SCH_0
	default "GPIO_0"

config GPIO_SCH_0_BASE_ADDR
	hex "SCH GPIO base address"
	depends on GPIO_SCH_0
	help
	  The memory address where the memory mapped registers are found

config GPIO_SCH_0_BITS
	int "Total of GPIO pins controlled"
	depends on GPIO_SCH_0
	help
	  The total of pins the controller can manage (CPU dependent)

config GPIO_SCH_1
	bool "Enable SCH GPIO port 1"
	default n
	depends on GPIO_SCH

config GPIO_SCH_1_DEV_NAME
	string "Name of the device"
	depends on GPIO_SCH_1
	default "GPIO_1"

config GPIO_SCH_1_BASE_ADDR
	hex "SCH GPIO base address"
	depends on GPIO_SCH_1
	help
	  The memory address where the memory mapped registers are found

config GPIO_SCH_1_BITS
	int "Total of GPIO pins controlled"
	depends on GPIO_SCH_1
	help
	  The total of pins the controller can manage (CPU dependent)

endif # GPIO_SCH
