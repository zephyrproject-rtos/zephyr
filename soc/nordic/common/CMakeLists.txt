# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_RISCV_CORE_NORDIC_VPR vpr)

if(CONFIG_ARM AND CONFIG_SOC_FAMILY_NORDIC_NRF)
  zephyr_library_sources(soc_nrf_common.S)
endif()

zephyr_library_sources_ifdef(CONFIG_POWEROFF poweroff.c)

zephyr_include_directories(.)

if(CONFIG_TFM_PARTITION_PLATFORM)
  zephyr_library_sources(soc_secure.c)
  zephyr_library_include_directories(
    $<TARGET_PROPERTY:tfm,TFM_BINARY_DIR>/api_ns/interface/include
  )
endif()
