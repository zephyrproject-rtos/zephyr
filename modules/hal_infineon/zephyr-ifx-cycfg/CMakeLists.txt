# Copyright (c) 2025 Infineon Technologies AG,
# or an affiliate of Infineon Technologies AG.
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_FAMILY_INFINEON_EDGE)
  set(zephyr_ifx_cycfg_dir ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/zephyr-ifx-cycfg/kit_pse84_eval)

  zephyr_include_directories(${zephyr_ifx_cycfg_dir})
  zephyr_library_sources(${zephyr_ifx_cycfg_dir}/cycfg_qspi_memslot.c)
endif()
