# Copyright 2024 NXP
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_MIMX9352)
  add_subdirectory(imx93)
elseif(CONFIG_SOC_MIMX9596)
  add_subdirectory(imx95)
endif()
