# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(DEFINED CONFIG_SOC_SERIES)
  add_subdirectory(${CONFIG_SOC_SERIES})
else()
  add_subdirectory(${CONFIG_SOC})
endif()
