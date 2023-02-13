#
# Copyright (c) 2020, STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories_ifdef(CONFIG_OPENAMP_RSC_TABLE .)
zephyr_sources_ifdef(CONFIG_OPENAMP_RSC_TABLE resource_table.c)

zephyr_compile_definitions_ifdef(CONFIG_OPENAMP_WITH_DCACHE
				 WITH_DCACHE_VRINGS
				 WITH_DCACHE_BUFFERS)
