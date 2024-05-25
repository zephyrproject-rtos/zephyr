# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_RISCV_CORE_NORDIC_VPR vpr)

zephyr_linker_sources_ifdef(CONFIG_ARM SECTIONS arm_platform_init.ld)

zephyr_library_sources_ifdef(CONFIG_POWEROFF poweroff.c)

zephyr_include_directories(.)

if(CONFIG_TFM_PARTITION_PLATFORM)
  zephyr_library_sources(soc_secure.c)
  zephyr_library_include_directories(
    $<TARGET_PROPERTY:tfm,TFM_BINARY_DIR>/api_ns/interface/include
  )
endif()
