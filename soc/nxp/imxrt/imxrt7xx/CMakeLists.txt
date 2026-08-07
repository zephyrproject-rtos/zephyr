#
# Copyright 2024-2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_SOC_MIMXRT798S_CM33_CPU0 OR CONFIG_SOC_MIMXRT798S_CM33_CPU1)
  add_subdirectory(cm33)
elseif(CONFIG_SOC_MIMXRT798S_HIFI4)
  add_subdirectory(hifi4)
elseif(CONFIG_SOC_MIMXRT798S_HIFI1)
  add_subdirectory(hifi1)
endif()
