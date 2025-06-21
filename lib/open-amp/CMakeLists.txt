#
# Copyright (c) 2020, STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories_ifdef(CONFIG_OPENAMP_RSC_TABLE .)
zephyr_sources_ifdef(CONFIG_OPENAMP_RSC_TABLE resource_table.c)

# include address translation files
zephyr_include_directories_ifdef(CONFIG_OPENAMP_VENDOR_ADDR_TRANSLATION vendor)
zephyr_sources_ifdef(CONFIG_OPENAMP_RSC_TABLE addr_translation.c)
