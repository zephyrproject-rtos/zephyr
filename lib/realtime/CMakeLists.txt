# Copyright (c) 2024 Bjarki Arge Andreasen
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SYS_REALTIME)
  if(CONFIG_USERSPACE)
    zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/sys/realtime.h)
    zephyr_sources(handlers.c)
  endif()
  if(CONFIG_SYS_REALTIME_CONVERT_NATIVE)
    zephyr_sources(convert_native.c)
  endif()
  if(CONFIG_SYS_REALTIME_BASE_CLOCK_SYS_CLOCK)
    zephyr_sources(base_clock_sys_clock.c)
  endif()
endif()
