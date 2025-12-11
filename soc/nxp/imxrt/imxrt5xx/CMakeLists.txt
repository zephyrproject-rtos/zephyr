#
# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_SOC_MIMXRT595S_CM33)
  add_subdirectory(cm33)
elseif(CONFIG_SOC_MIMXRT595S_F1)
  add_subdirectory(f1)
endif()
