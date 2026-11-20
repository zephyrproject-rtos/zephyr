# Copyright (c) 2023 KNS Group LLC (YADRO)
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_PROFILING_PERF OR CONFIG_PROFILING_PERF_EVENTS)
  add_subdirectory(perf)
endif()
