# Copyright (c) 2025 Analog Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

zephyr_library()
zephyr_library_sources(cpu_freq.c)

# Policy selection
if(CONFIG_CPU_FREQ_POLICY_NONE)
  message(FATAL_ERROR "No CPU frequency policy selected. Please choose a valid policy in Kconfig.")
endif()

zephyr_sources_ifdef(CONFIG_CPU_FREQ_POLICY_ON_DEMAND policies/on_demand/on_demand.c)
