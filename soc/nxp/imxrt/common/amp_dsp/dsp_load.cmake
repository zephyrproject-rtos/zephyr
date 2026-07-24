# Copyright 2025-2026 NXP
# SPDX-License-Identifier: Apache-2.0

# Embed the DSP remote image into the primary domain and provide the
# dsp_start() function that loads and boots the DSP.
#
# This adds the shared dsp.c / dspimgs.S sources to the primary image, points
# the include path at the shared dsp.h header, and wires the sliced remote
# binaries (produced by nxp_rtxxx_dsp_remote_imgs()) into dspimgs.S via compile
# definitions. The remote image is expected to have been added as a sysbuild
# ExternalZephyrProject named "remote".
function(nxp_imxrt_amp_dsp_load)
  set(dsp_build_dir "${APPLICATION_BINARY_DIR}/../remote/zephyr")
  set(dsp_dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}")

  target_sources(app PRIVATE
    "${dsp_dir}/src/dsp.c"
    "${dsp_dir}/src/dspimgs.S"
  )
  target_include_directories(app PRIVATE "${dsp_dir}/include")

  set(dsp_bin_reset "${dsp_build_dir}/zephyr.reset.bin")
  set(dsp_bin_text  "${dsp_build_dir}/zephyr.text.bin")
  set(dsp_bin_data  "${dsp_build_dir}/zephyr.data.bin")

  target_compile_definitions(app PRIVATE
    "DSP_BIN_RESET=\"${dsp_bin_reset}\""
    "DSP_BIN_TEXT=\"${dsp_bin_text}\""
    "DSP_BIN_DATA=\"${dsp_bin_data}\""
  )

  set_source_files_properties(
    "${dsp_dir}/src/dspimgs.S"
    TARGET_DIRECTORY app
    PROPERTIES
    OBJECT_DEPENDS "${dsp_bin_reset};${dsp_bin_text};${dsp_bin_data}"
  )
endfunction()
