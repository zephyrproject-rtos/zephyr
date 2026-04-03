# Copyright 2024 Daniel DeGrasse <daniel@degrasse.com>
# SPDX-License-Identifier: Apache-2.0

if(SB_CONFIG_SOC_SERIES_IMXRT11XX)
  # Include RT11XX specific sysbuild
  include(${SOC_${SB_CONFIG_SOC}_DIR}/imxrt11xx/sysbuild.cmake OPTIONAL)
endif()
