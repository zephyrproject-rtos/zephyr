# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_S32K566_M7)
  add_subdirectory(m7)
endif()

if(CONFIG_SOC_S32K566_R52)
  add_subdirectory(r52)
endif()

zephyr_library_sources_ifdef(CONFIG_WDOG_INIT watchdog_init.c)
