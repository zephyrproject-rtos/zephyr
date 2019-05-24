# Copyright (c) 2019 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_sources(
  soc.c
  )
zephyr_sources_ifdef(CONFIG_RPROC_RSC_TABLE resource_table.c)
