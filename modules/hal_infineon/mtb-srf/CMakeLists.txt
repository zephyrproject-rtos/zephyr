# SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
# or an affiliate of Infineon Technologies AG. All rights reserved.</text>
#
# SPDX-License-Identifier: Apache-2.0

set(mtb_srf_dir ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/mtb-srf)
set(mtb_srf_ns_inc ${mtb_srf_dir}/include/COMPONENT_NON_SECURE_DEVICE)

zephyr_include_directories(${mtb_srf_dir}/include)
zephyr_library_sources(${mtb_srf_dir}/source/mtb_srf.c)

if(NOT DEFINED CONFIG_TRUSTED_EXECUTION_SECURE)
  zephyr_include_directories(${mtb_srf_ns_inc})
endif()

if(NOT (CONFIG_PSOC_EDGE_M55_SRF_SUPPORT OR CONFIG_BUILD_WITH_TFM))
  return()
endif()

zephyr_compile_definitions(IFX_MTB_SRF MTB_SRF_USE_PSA)
zephyr_library_sources(${mtb_srf_dir}/source/COMPONENT_NON_SECURE_DEVICE/mtb_srf_pool.c)

if(CONFIG_CPU_CORTEX_M33)
  zephyr_compile_definitions(MTB_SRF_CUSTOM_PROCESS_REQUEST)
endif()

if(CONFIG_PSOC_EDGE_M55_SRF_SUPPORT)
  # If using TF-M we need the custom handling for SRF requests of IPC type
  zephyr_compile_definitions(
    MTB_SRF_IPC_CUSTOM_PACKET_TYPE
    MTB_SRF_CUSTOM_IPC_REQUEST_SUBMIT
    CY_RTOS_AWARE
  )
  zephyr_include_directories(${mtb_srf_ns_inc}/COMPONENT_MW_MTB_IPC)
  zephyr_library_sources(${mtb_srf_dir}/source/COMPONENT_NON_SECURE_DEVICE/COMPONENT_MW_MTB_IPC/mtb_srf_ipc.c)
endif()
