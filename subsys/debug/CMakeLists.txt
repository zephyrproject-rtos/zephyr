# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(
  CONFIG_OPENOCD_SUPPORT
  openocd.c
  )

zephyr_sources_ifdef(
  CONFIG_ASAN
  asan_hacks.c
  )

zephyr_sources_ifdef(
  CONFIG_THREAD_ANALYZER
  thread_analyzer.c
  )
