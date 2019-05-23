# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories_ifdef(CONFIG_PROFILING include)

zephyr_sources_ifdef(
  CONFIG_PROFILING
  profiling.c
  )
