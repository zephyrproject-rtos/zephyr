# Copyright (c) 2021 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_SERIES)
  add_subdirectory(${CONFIG_SOC_SERIES})
else()
  add_subdirectory(${CONFIG_SOC})
endif()
add_subdirectory(common)
zephyr_include_directories(common)
