# Copyright (c) 2023 KNS Group LLC (YADRO)
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_PROFILING_PERF)
  add_subdirectory(backends)
endif()

zephyr_library()

zephyr_library_sources(
  perf.c
)
