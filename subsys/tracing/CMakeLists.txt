# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(
  CONFIG_TRACING_CPU_STATS
  cpu_stats.c
  )

zephyr_sources_ifdef(
  CONFIG_TRACING_CORE
  tracing_buffer.c
  tracing_core.c
  tracing_format_common.c
  )
if(CONFIG_TRACING_CORE)
zephyr_sources_ifdef(
  CONFIG_TRACING_SYNC
  tracing_format_sync.c
  )

zephyr_sources_ifdef(
  CONFIG_TRACING_ASYNC
  tracing_format_async.c
  )

zephyr_sources_ifdef(
  CONFIG_TRACING_BACKEND_USB
  tracing_backend_usb.c
  )

zephyr_sources_ifdef(
  CONFIG_TRACING_BACKEND_UART
  tracing_backend_uart.c
  )

zephyr_sources_ifdef(
  CONFIG_TRACING_BACKEND_POSIX
  tracing_backend_posix.c
  )
endif()

zephyr_include_directories_ifdef(
  CONFIG_TRACING
  ${ZEPHYR_BASE}/kernel/include
  ${ARCH_DIR}/${ARCH}/include
)

zephyr_include_directories_ifdef(CONFIG_TRACING include)

add_subdirectory_ifdef(CONFIG_TRACING_CTF ctf)

add_subdirectory_ifdef(CONFIG_SEGGER_SYSTEMVIEW sysview)
