# Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
#
# SPDX-License-Identifier: Apache-2.0

zephyr_linker_sources_ifdef(CONFIG_PROFILING_PERF_EVENTS ROM_SECTIONS sections.ld)

if(CONFIG_PROFILING_PERF_EVENTS)
  zephyr_iterable_section(NAME perf_event_provider GROUP RODATA_REGION)
endif()

zephyr_library_sources_ifdef(CONFIG_PROFILING_PERF_EVENTS
  core.c
)

zephyr_library_sources_ifdef(CONFIG_PROFILING_PERF_EVENTS_SHELL
  shell.c
)

zephyr_library_sources_ifdef(CONFIG_PROFILING_PERF_EVENT_PROVIDER_CPU
  providers/cpu.c
)
