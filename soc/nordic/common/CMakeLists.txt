# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_RISCV_CORE_NORDIC_VPR vpr)

# Let SystemInit() be called in place of soc_reset_hook() by default.
zephyr_linker_symbol(SYMBOL soc_reset_hook EXPR "@SystemInit@")

# This file is used when the CMake linker script generator is disabled.
zephyr_linker_sources(SECTIONS platform_init.ld)

# TF-M provides its own reboot sequence
if(NOT CONFIG_TFM_PARTITION_PLATFORM)
  zephyr_library_sources_ifdef(CONFIG_ARM reboot.c)
endif()

zephyr_library_sources_ifdef(CONFIG_POWEROFF poweroff.c)
if(CONFIG_ARM)
  zephyr_library_sources_ifdef(CONFIG_NRF_PLATFORM_HALTIUM soc_lrcconf.c)
endif()

if((CONFIG_SOC_SERIES_NRF54HX OR CONFIG_SOC_SERIES_NRF92X) AND CONFIG_CPU_HAS_CUSTOM_FIXED_SOC_MPU_REGIONS)
  zephyr_library_sources(nrf54hx_nrf92x_mpu_regions.c)
endif()

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

zephyr_library_sources_ifdef(CONFIG_NRF_SYS_EVENT nrf_sys_event.c)
zephyr_library_sources_ifdef(CONFIG_MRAM_LATENCY mram_latency.c)
