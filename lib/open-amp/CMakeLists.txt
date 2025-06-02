#
# Copyright (c) 2020, STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories_ifdef(CONFIG_OPENAMP_RSC_TABLE .)
zephyr_sources_ifdef(CONFIG_OPENAMP_RSC_TABLE resource_table.c)
# include address translation files
if(CONFIG_OPENAMP_ADDR_TRANSLATION)
  zephyr_include_directories(addr_translation)
  zephyr_sources(addr_translation/addr_translation.c)
  # include vendor-specific address translation table
  zephyr_include_directories(addr_translation/vendor_addr_translation)
endif()
