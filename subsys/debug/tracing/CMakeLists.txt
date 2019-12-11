# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SEGGER_SYSTEMVIEW OR CONFIG_TRACING_CPU_STATS)
  zephyr_library()

  zephyr_library_sources_ifdef(
    CONFIG_SEGGER_SYSTEMVIEW
    sysview_config.c
    sysview.c
    )

  zephyr_library_sources_ifdef(
    CONFIG_TRACING_CPU_STATS
    cpu_stats.c
    )

  if(CONFIG_TRACING)
    zephyr_library_include_directories(
      ${ZEPHYR_BASE}/kernel/include
      ${ZEPHYR_BASE}/arch/${ARCH}/include
      )
  endif()
endif()

zephyr_include_directories_ifdef(CONFIG_TRACING include)

add_subdirectory_ifdef(CONFIG_TRACING_CTF ctf)
