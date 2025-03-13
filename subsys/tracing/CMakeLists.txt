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

if (CONFIG_TRACING_BACKEND_POSIX)
  zephyr_sources(tracing_backend_posix.c)
  if (CONFIG_NATIVE_APPLICATION)
    zephyr_library_sources(tracing_backend_posix_bottom.c)
  else()
    target_sources(native_simulator INTERFACE tracing_backend_posix_bottom.c)
  endif()
endif()

zephyr_sources_ifdef(
  CONFIG_TRACING_BACKEND_RAM
  tracing_backend_ram.c
  )

zephyr_sources_ifdef(
  CONFIG_TRACING_BACKEND_ADSP_MEMORY_WINDOW
  tracing_backend_adsp_memory_window.c
  )

endif()

zephyr_sources(
  tracing_none.c
  )

zephyr_sources_ifdef(
  CONFIG_TRACING_OBJECT_TRACKING
  tracing_tracking.c
  )

zephyr_include_directories_ifdef(
  CONFIG_TRACING
  ${ZEPHYR_BASE}/kernel/include
  ${ARCH_DIR}/${ARCH}/include
)

zephyr_include_directories_ifdef(CONFIG_TRACING include)

add_subdirectory_ifdef(CONFIG_TRACING_CTF ctf)
add_subdirectory_ifdef(CONFIG_SEGGER_SYSTEMVIEW sysview)
add_subdirectory_ifdef(CONFIG_TRACING_TEST test)
add_subdirectory_ifdef(CONFIG_TRACING_USER user)
