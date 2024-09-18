# Copyright (c) 2024 Meta Platforms
#
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_PROFILING_PERF_BACKEND_RISCV
  perf_riscv.c
)

zephyr_sources_ifdef(CONFIG_PROFILING_PERF_BACKEND_X86
  perf_x86.c
)

zephyr_sources_ifdef(CONFIG_PROFILING_PERF_BACKEND_X86_64
  perf_x86_64.c
)
