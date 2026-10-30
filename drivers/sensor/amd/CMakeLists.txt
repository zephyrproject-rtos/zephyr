# Copyright (c) 2026 Advanced Micro Devices, Inc. (AMD)
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_VERSAL_SYSMON)
  zephyr_library()
  zephyr_library_sources(versal_sysmon.c)
  zephyr_include_directories(.)
endif()
