# Copyright (c) 2026 Endress+Hauser GmbH+Co. KG
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

module = DISK_FTL
module-str = disk_ftl
source "subsys/logging/Kconfig.template.log_config"

endif # DISK_DRIVER_FTL
