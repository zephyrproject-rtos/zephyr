# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_RISCV_CORE_NORDIC_VPR vpr)

if(CONFIG_ARM)
  # Let SystemInit() be called in place of z_arm_platform_init() by default.
  zephyr_linker_symbol(SYMBOL z_arm_platform_init EXPR "@SystemInit@")

  # This file is used when the CMake linker script generator is disabled.
  zephyr_linker_sources(SECTIONS arm_platform_init.ld)
endif()

zephyr_library_sources_ifdef(CONFIG_POWEROFF poweroff.c)

zephyr_include_directories(.)

if(CONFIG_HAS_NORDIC_DMM)
  zephyr_library_sources(dmm.c)
endif()

if(CONFIG_TFM_PARTITION_PLATFORM)
  zephyr_library_sources(soc_secure.c)
  zephyr_library_include_directories(
    $<TARGET_PROPERTY:tfm,TFM_BINARY_DIR>/api_ns/interface/include
  )
endif()
