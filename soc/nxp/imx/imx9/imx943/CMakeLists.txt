# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

if(${CONFIG_MCUX_CORE_SUFFIX} STREQUAL "_ca55")
  add_subdirectory(a55)
endif()
