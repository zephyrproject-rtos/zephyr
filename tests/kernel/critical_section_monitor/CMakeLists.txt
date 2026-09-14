# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(kernel_critical_section_monitor)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/kernel/include)
target_sources(app PRIVATE src/main.c src/timing.c)
target_sources_ifdef(CONFIG_SHELL_BACKEND_DUMMY app PRIVATE src/shell.c)

if(CONFIG_SHELL_BACKEND_DUMMY)
  target_link_options(app PUBLIC -Wl,--wrap=z_critical_section_monitor_stats_get)
endif()

# Observe scheduler/idle handoffs and control timing without changing production hooks.
target_link_options(app PUBLIC
  -Wl,--wrap=sys_clock_cycle_get_32
  -Wl,--wrap=arch_cpu_idle
  -Wl,--wrap=arch_cpu_atomic_idle
  -Wl,--wrap=z_pend_curr
  -Wl,--wrap=z_critical_section_monitor_spin_released
)
