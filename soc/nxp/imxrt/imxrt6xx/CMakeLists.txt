#
# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_SOC_MIMXRT685S_CM33)
  add_subdirectory(cm33)
endif()

if(CONFIG_SOC_MIMXRT685S_HIFI4)
  add_subdirectory(hifi4)
endif()
