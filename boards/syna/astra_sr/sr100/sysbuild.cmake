# SPDX-FileCopyrightText: Copyright (c) 2026 Synaptics Incorporated
# SPDX-License-Identifier: Apache-2.0

if(SB_CONFIG_SOC_SR100_M4_APP_DIR AND NOT SB_CONFIG_SOC_SR100_M4_APP_DIR STREQUAL "")
  include(ExternalProject)

  ExternalZephyrProject_Add(
    APPLICATION sr100_m4
    SOURCE_DIR ${SB_CONFIG_SOC_SR100_M4_APP_DIR}
    BOARD sr100_rdk/sr100/m4
  )

  set(SR100_IMAGEGEN_M4_PARAM "-m4_image" "${CMAKE_BINARY_DIR}/sr100_m4/zephyr/zephyr.elf")

  # Build and flash ordering for dual-core image composition.
  add_dependencies(${DEFAULT_IMAGE} sr100_m4)
  sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} sr100_m4)
  sysbuild_add_dependencies(FLASH ${DEFAULT_IMAGE} sr100_m4)

  # Propagate reset policy to the M55 runtime image.
  set_config_bool(${DEFAULT_IMAGE} CONFIG_SOC_SR100_M4_RELEASE_RESET y)
endif()

set(SR100_IMAGEGEN_M55_IMG "${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/zephyr.elf")

if(NOT DEFINED TC_RUNID)
  include(${CMAKE_CURRENT_LIST_DIR}/image_generation.cmake)
endif()
