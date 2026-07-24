# Copyright 2025-2026 NXP
# SPDX-License-Identifier: Apache-2.0

include(${CMAKE_CURRENT_LIST_DIR}/dsp_board_select.cmake)

# Add the DSP remote image to a sysbuild build.
#
#   nxp_rtxxx_add_dsp_remote(<source_dir> [IMAGE <name>])
#
# <source_dir>   Path to the DSP (HiFi4) application source directory.
# IMAGE <name>   Name of the ExternalZephyrProject image (default: "remote").
#
# The remote board is resolved automatically from the primary board, the remote
# image is built first (the primary image embeds its output), and
# CONFIG_NXP_IMXRT_AMP_DSP is enabled on both the primary and the remote image
# so that the SoC CMakeLists adds the embed / objcopy steps automatically.
function(nxp_imxrt_add_amp_dsp_remote source_dir)
  cmake_parse_arguments(ARG "" "IMAGE" "" ${ARGN})

  set(image "remote")
  if(ARG_IMAGE)
    set(image "${ARG_IMAGE}")
  endif()

  nxp_rtxxx_amp_dsp_remote_board(remote_board)

  ExternalZephyrProject_Add(
    APPLICATION ${image}
    SOURCE_DIR ${source_dir}
    BOARD ${remote_board}
    BUILD_ONLY TRUE
  )

  add_dependencies(${DEFAULT_IMAGE} ${image})
  sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} ${image})

  # Enable the DSP embed step on the primary image and the objcopy slicing step
  # on the remote image.
  set_config_bool(${DEFAULT_IMAGE} CONFIG_NXP_IMXRT_AMP_DSP y)
  set_config_bool(${image} CONFIG_NXP_IMXRT_AMP_DSP y)
endfunction()
