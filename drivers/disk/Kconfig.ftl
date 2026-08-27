# Copyright (c) 2025 Endress+Hauser GmbH+Co. KG
# SPDX-License-Identifier: Apache-2.0

config DISK_DRIVER_FTL
	bool "Flash translation layer"
	default y
	depends on ZEPHYR_DHARA_MODULE
	depends on DT_HAS_ZEPHYR_FTL_DHARA_ENABLED
	select FLASH
	select FLASH_MAP
	select FLASH_EX_OP_ENABLED
	help
	  Enable flash translation layer disk driver for NAND flashes.

if DISK_DRIVER_FTL

config DISK_FTL_GC_RATIO
	int "Garbage collection ratio"
	default 15
	help
	  This is the ratio of garbage collection operations to real writes when
	  automatic collection is active. Smaller values lead to faster and more
	  predictable input/output at the expense of capacity.

config DISK_FTL_SUPPORT_CONCURRENT_ACCESS
	bool "Support concurrent access"
	help
	  Enable support for concurrent access to the disk. This allows multiple threads
	  to access the disk at the same time.

module = DISK_FTL
module-str = disk_ftl
source "subsys/logging/Kconfig.template.log_config"

endif # DISK_DRIVER_FTL
