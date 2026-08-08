# Copyright 2024 Daniel DeGrasse <daniel@degrasse.com>
# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

if(SB_CONFIG_SOC_SERIES_IMXRT11XX)
  # Include RT11XX specific sysbuild
  include(${SOC_${SB_CONFIG_SOC}_DIR}/imxrt11xx/sysbuild.cmake OPTIONAL)
endif()

# Include helper for projects using DSP.
include(${SOC_${SB_CONFIG_SOC}_DIR}/common/amp_dsp/sysbuild.cmake OPTIONAL)

if(SB_CONFIG_NXP_IMXRT_BUILD_AMP_DSP)
  # Default DSP remote application location; apps with a differing layout
  # (e.g. multiple remotes) can call nxp_rtxxx_add_dsp_remote() themselves
  # instead of setting SB_CONFIG_NXP_IMXRT_BUILD_DSP.
  nxp_imxrt_add_amp_dsp_remote(${APP_DIR}/remote)
endif()
