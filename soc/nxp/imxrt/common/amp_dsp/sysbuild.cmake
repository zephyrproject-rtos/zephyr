# Copyright 2025-2026 NXP
# SPDX-License-Identifier: Apache-2.0

include(${CMAKE_CURRENT_LIST_DIR}/dsp_board_select.cmake)

# Add the DSP remote image to a sysbuild build.
#
#   nxp_imxrt_add_amp_dsp_remote(<source_dir> [IMAGE <name>])
#
# <source_dir>   Path to the DSP (HiFi4) application source directory.
# IMAGE <name>   ExternalZephyrProject image name. Defaults to the remote
#                board's CPU cluster qualifier (e.g. "hifi4").
#
# The two domains share no build artifact: the primary only reads the remote's
# stock zephyr_image_info.h to learn each segment's LMA/VMA and copies flash to
# RAM at runtime in dsp_start().
function(nxp_imxrt_add_amp_dsp_remote source_dir)
  cmake_parse_arguments(ARG "" "IMAGE" "" ${ARGN})

  nxp_rtxxx_amp_dsp_remote_board(remote_board)

  if(ARG_IMAGE)
    set(image "${ARG_IMAGE}")
  else()
    string(REGEX REPLACE ".*/" "" image "${remote_board}")
  endif()

  # The DSP remote is a normal sysbuild domain. Its loadable segments have their
  # load addresses (LMAs) in the shared FlexSPI window, so its own .hex is
  # programmed by "west flash" through the primary core's flash runner (the
  # HiFi4 has no flash loader of its own; see the board.cmake flash-device
  # override). This keeps the two domains decoupled - nothing is merged.
  ExternalZephyrProject_Add(
    APPLICATION ${image}
    SOURCE_DIR ${source_dir}
    BOARD ${remote_board}
  )

  # Build the remote first: the primary consumes its image information header.
  add_dependencies(${DEFAULT_IMAGE} ${image})
  sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} ${image})

  # Remote cache is not populated yet, so derive the path (as SECOND_CORE_MCUX
  # does) instead of sysbuild_get(... CACHE).
  set(remote_public_dir "${APPLICATION_BINARY_DIR}/${image}/zephyr/include/public")

  set_config_bool(${image} CONFIG_SOC_SERIES_IMXRT_AMP_DSP y)
  set_config_bool(${DEFAULT_IMAGE} CONFIG_SOC_SERIES_IMXRT_AMP_DSP y)
  set_config_string(${DEFAULT_IMAGE} CONFIG_SOC_SERIES_IMXRT_AMP_DSP_REMOTE_DIR
    "${remote_public_dir}")

  # The remote emits the image information header the primary reads to learn
  # each segment's LMA/VMA. This header is the only build-time artifact shared
  # between the two domains.
  set_config_bool(${image} CONFIG_BUILD_OUTPUT_INFO_HEADER y)
endfunction()
