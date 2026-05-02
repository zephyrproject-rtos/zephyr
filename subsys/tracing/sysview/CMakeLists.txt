# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(
  CONFIG_SEGGER_SYSTEMVIEW
  sysview_config.c
  sysview.c
  )

zephyr_include_directories(.)
