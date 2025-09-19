# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

if(CONFIG_SOC_MIMX94398_A55)
  add_subdirectory(a55)
elseif(CONFIG_SOC_MIMX94398_M33)
  add_subdirectory(m33)
elseif(CONFIG_SOC_MIMX94398_M7_0)
  add_subdirectory(m7_0)
elseif(CONFIG_SOC_MIMX94398_M7_1)
  add_subdirectory(m7_1)
endif()
