# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(
  CONFIG_DEBUG_THREAD_INFO
  thread_info.c
  )

zephyr_sources_ifdef(
  CONFIG_ASAN
  asan_hacks.c
  )

zephyr_sources_ifdef(
  CONFIG_THREAD_ANALYZER
  thread_analyzer.c
  )

add_subdirectory_ifdef(
  CONFIG_DEBUG_COREDUMP
  coredump
  )

zephyr_sources_ifdef(
  CONFIG_GDBSTUB
  gdbstub.c
  )

zephyr_sources_ifdef(
  CONFIG_GDBSTUB_SERIAL_BACKEND
  gdbstub/gdbstub_backend_serial.c
  )
