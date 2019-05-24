# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories_ifdef(CONFIG_TRACING include)

zephyr_sources_ifdef(
  CONFIG_SEGGER_SYSTEMVIEW
  sysview_config.c
  sysview.c
  )

zephyr_sources_ifdef(
  CONFIG_TRACING_CPU_STATS
  cpu_stats.c
  )

add_subdirectory_ifdef(CONFIG_TRACING_CTF ctf)
