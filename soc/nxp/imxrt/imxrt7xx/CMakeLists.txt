#
# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_SOC_MIMXRT798S_HIFI4)
  add_subdirectory(hifi4)
elseif(CONFIG_SOC_MIMXRT798S_HIFI1)
  add_subdirectory(hifi1)
endif()
