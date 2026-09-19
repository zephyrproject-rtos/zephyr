# Copyright 2025-2026 NXP
# SPDX-License-Identifier: Apache-2.0

# Add the dsp_start() loader to the primary (ARM) domain. It learns the DSP
# remote's segment load addresses solely from the remote's stock
# zephyr_image_info.h, reached via CONFIG_SOC_SERIES_IMXRT_AMP_DSP_REMOTE_DIR
# rather than a hardcoded path into the remote build tree. This header is the
# only build-time coupling between the two domains; the DSP image itself is a
# normal sysbuild domain that is flashed on its own (see domains.yaml), so no
# image reaches into another's artifacts and nothing is merged here.
function(nxp_imxrt_amp_dsp_load)
  set(dsp_dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}")
  set(remote_dir "${CONFIG_SOC_SERIES_IMXRT_AMP_DSP_REMOTE_DIR}")

  target_sources(app PRIVATE "${dsp_dir}/src/dsp.c")
  target_include_directories(app PRIVATE "${dsp_dir}/include" "${remote_dir}")
endfunction()
