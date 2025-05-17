#
# Copyright (c) 2020, STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories_ifdef(CONFIG_OPENAMP_RSC_TABLE .)
# include vendor-specific resource table files
if(CONFIG_OPENAMP_VENDOR_RSC_TABLE_ENTRY)
  zephyr_include_directories(vendor_rsc_table)
  zephyr_sources(vendor_rsc_table/${CONFIG_OPENAMP_VENDOR_RSC_TABLE_FILE})
elseif(CONFIG_OPENAMP_RSC_TABLE)
  zephyr_sources(resource_table.c)
endif()
