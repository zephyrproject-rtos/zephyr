# SPDX-FileCopyrightText: Copyright 2026 Arm Limited and/or its
# SPDX-FileCopyrightText: affiliates <open-source-office@arm.com>
#
# SPDX-License-Identifier: Apache-2.0

zephyr_linker_sources_ifdef(CONFIG_PROFILING_PERF_EVENTS SECTIONS sections.ld)

zephyr_library_sources_ifdef(CONFIG_PROFILING_PERF_EVENTS
  core.c
)

zephyr_library_sources_ifdef(CONFIG_PROFILING_PERF_EVENT_PROVIDER_CPU
  providers/cpu.c
)
