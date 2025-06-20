#
# Copyright (c) 2020, STMicroelectronics
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

# include vendor-specific resources
if(CONFIG_OPENAMP_VENDOR_RSC_TABLE OR CONFIG_OPENAMP_VENDOR_ADDR_TRANSLATION)
  zephyr_include_directories(vendor)
endif()

if(CONFIG_OPENAMP_RSC_TABLE)
  zephyr_include_directories(.)
  # include generic address translation ops
  zephyr_sources(addr_translation.c)
  # include vendor-specific resource table files
  if(CONFIG_OPENAMP_VENDOR_RSC_TABLE)
    zephyr_sources(vendor/${CONFIG_OPENAMP_VENDOR_RSC_TABLE_FILE})
  else()
    # include generic resource table
    zephyr_sources(resource_table.c)
  endif()
endif()
