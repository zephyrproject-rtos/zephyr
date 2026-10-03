# Copyright 2024 Daniel DeGrasse <daniel@degrasse.com>
# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

if(SB_CONFIG_SOC_SERIES_IMXRT11XX)
  # Include RT11XX specific sysbuild
  include(${SOC_${SB_CONFIG_SOC}_DIR}/imxrt11xx/sysbuild.cmake OPTIONAL)
endif()

# Include helper for projects using DSP.
include(${SOC_${SB_CONFIG_SOC}_DIR}/common/amp_dsp/sysbuild.cmake OPTIONAL)

if(SB_CONFIG_SOC_SERIES_IMXRT_BUILD_AMP_DSP)
  # DSP remote application location, selectable via Kconfig. Apps with a
  # differing layout (e.g. multiple remotes) can call
  # nxp_imxrt_add_amp_dsp_remote() themselves instead.
  set(dsp_remote_dir "${SB_CONFIG_SOC_SERIES_IMXRT_BUILD_AMP_DSP_REMOTE_DIR}")
  if(NOT IS_ABSOLUTE "${dsp_remote_dir}")
    set(dsp_remote_dir "${APP_DIR}/${dsp_remote_dir}")
  endif()
  nxp_imxrt_add_amp_dsp_remote(${dsp_remote_dir})
endif()
