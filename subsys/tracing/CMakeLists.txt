# SPDX-License-Identifier: Apache-2.0

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

zephyr_sources_ifdef(
  CONFIG_TRACING_BACKEND_RAM
  tracing_backend_ram.c
  )

endif()

if(NOT CONFIG_PERCEPIO_TRACERECORDER AND NOT CONFIG_TRACING_CTF
  AND NOT CONFIG_SEGGER_SYSTEMVIEW AND NOT CONFIG_TRACING_TEST)
  zephyr_sources(tracing_none.c)
endif()

zephyr_include_directories_ifdef(
  CONFIG_TRACING
  ${ZEPHYR_BASE}/kernel/include
  ${ARCH_DIR}/${ARCH}/include
)

zephyr_include_directories_ifdef(CONFIG_TRACING include)

add_subdirectory_ifdef(CONFIG_TRACING_CTF ctf)
add_subdirectory_ifdef(CONFIG_SEGGER_SYSTEMVIEW sysview)
add_subdirectory_ifdef(CONFIG_TRACING_TEST test)
