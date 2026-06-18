# SPDX-FileCopyrightText: Copyright (c) 2026 Synaptics Incorporated
# SPDX-License-Identifier: Apache-2.0

if(SB_CONFIG_SR100_INCLUDE_M4_IMAGE)
  if(NOT(SR100_M4_APP_DIR))
    message(FATAL_ERROR "Missing SR100 M4 application")
  endif()

  ExternalZephyrProject_Add(
    APPLICATION sr100_m4
    SOURCE_DIR ${SR100_M4_APP_DIR}
    BOARD sr100_rdk/sr100/m4
  )

  sysbuild_cache_set(
    VAR SR100_M4_ELF
    ${CMAKE_BINARY_DIR}/sr100_m4/zephyr/zephyr.elf
  )

  # Build and flash ordering for dual-core image composition.
  add_dependencies(${DEFAULT_IMAGE} sr100_m4)
  sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} sr100_m4)
  sysbuild_add_dependencies(FLASH ${DEFAULT_IMAGE} sr100_m4)

  # If MCUboot is enabled, flash it before both application images.
  if(SB_CONFIG_BOOTLOADER_MCUBOOT)
    sysbuild_add_dependencies(FLASH ${DEFAULT_IMAGE} mcuboot)
    sysbuild_add_dependencies(FLASH sr100_m4 mcuboot)
  endif()

  # Propagate reset policy to the M55 runtime image.
  set_config_bool(${DEFAULT_IMAGE} CONFIG_SR100_RELEASE_M4_RESET y)
endif()
