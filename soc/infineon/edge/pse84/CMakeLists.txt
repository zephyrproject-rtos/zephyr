# SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
# or an affiliate of Infineon Technologies AG. All rights reserved.</text>
#
# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/soc/infineon/edge/pse84/pse84_metadata.cmake)

if(CONFIG_BUILD_WITH_TFM)
  set_property(TARGET zephyr_property_target APPEND PROPERTY TFM_CMAKE_OPTIONS
    -DIFX_EPC=epc2
    -DIFX_UART_ENABLED=OFF
    # Use local hal_infineon assets instead of fetching from GitHub
    -DIFX_CORE_LIB_PATH=${ZEPHYR_HAL_INFINEON_MODULE_DIR}/core-lib
    -DIFX_DEV_SUPPORT_LIB_PATH=${ZEPHYR_HAL_INFINEON_MODULE_DIR}/mtb-dsl-pse8xxgp
    -DIFX_MTB_SRF_LIB_PATH=${ZEPHYR_HAL_INFINEON_MODULE_DIR}/mtb-srf
    -DIFX_ABSTRACTION_RTOS_LIB_PATH=${ZEPHYR_HAL_INFINEON_MODULE_DIR}/abstraction-rtos
    -DIFX_MTB_IPC_LIB_PATH=${ZEPHYR_HAL_INFINEON_MODULE_DIR}/mtb-ipc
    -DIFX_DEVICE_DB_LIB_PATH=${ZEPHYR_HAL_INFINEON_MODULE_DIR}/device-db
    -DIFX_BSP_LIB_PATH=${ZEPHYR_HAL_INFINEON_MODULE_DIR}/mtb-template-pse8xxgp/files
    -DIFX_SE_RT_SERVICES_UTILS_PATH=${ZEPHYR_HAL_INFINEON_MODULE_DIR}/se-rt-services-utils
  )

  if(CONFIG_PSOC_EDGE_M55_SRF_SUPPORT)
    set_property(TARGET zephyr_property_target APPEND PROPERTY TFM_CMAKE_OPTIONS
      -DIFX_MTB_MAILBOX=ON
      -DIFX_CM55_NS_PRESENT=ON
    )
  endif()

  if(CONFIG_BOARD_KIT_PSE84_AI)
    list(APPEND TFM_CMAKE_ARGS -DIFX_BSP_GENERATED_FILES_OUTPUT_PATH=${ZEPHYR_HAL_INFINEON_MODULE_DIR}/zephyr-ifx-cycfg/pse84/kit_pse84_ai)
    set(IFX_SMIF_0_MEMORY_CONFIG "S25HS512T_SMIF0_SlaveSlot_1")
  elseif(CONFIG_BOARD_KIT_PSE84_EVAL)
    list(APPEND TFM_CMAKE_ARGS -DIFX_BSP_GENERATED_FILES_OUTPUT_PATH=${ZEPHYR_HAL_INFINEON_MODULE_DIR}/zephyr-ifx-cycfg/pse84/kit_pse84_eval)
    set(IFX_SMIF_0_MEMORY_CONFIG "S25FS128S_SMIF0_SlaveSlot_1")
  else()
    message(FATAL_ERROR "No supported board selected.")
  endif()

endif()

if(CONFIG_CPU_CORTEX_M33 AND CONFIG_TRUSTED_EXECUTION_SECURE)
  zephyr_sources(soc_pse84_m33_s.c)

  zephyr_sources(security_config/pse84_s_mpc.c)
  zephyr_sources(security_config/pse84_s_protection.c)
  zephyr_sources(security_config/pse84_s_system.c)
  zephyr_sources(security_config/pse84_s_sau.c)

  set(unsigned_hex ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.hex)
  set(signed_hex ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.signed.hex)

  pse84_add_metadata_secure_hex(${unsigned_hex} ${signed_hex})
elseif(CONFIG_CPU_CORTEX_M33)
  zephyr_sources(soc_pse84_m33_ns.c)
elseif(CONFIG_CPU_CORTEX_M55)
  zephyr_sources(soc_pse84_m55.c)
endif()

zephyr_include_directories(security_config)
zephyr_sources(security_config/pse84_boot.c)

zephyr_sources(mpu_regions.c)

zephyr_sources_ifdef(CONFIG_PM power.c)
if(NOT CONFIG_TRUSTED_EXECUTION_NONSECURE)
  zephyr_sources_ifdef(CONFIG_REBOOT reboot.c)
endif()
zephyr_sources_ifdef(CONFIG_SOC_PSE84_LPM_TIMER lpm_timer.c)
zephyr_sources_ifdef(CONFIG_POWEROFF poweroff.c)

zephyr_include_directories(.)

if(${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "armclang")
  zephyr_library_sources(cy_syslib_ext.S)
endif()

# Add sections
if(CONFIG_CPU_CORTEX_M55)
zephyr_linker_sources(SECTIONS shared_mem_m55.ld)
else()
  if(CONFIG_TRUSTED_EXECUTION_SECURE)
    zephyr_linker_sources(SECTIONS shared_mem_sec.ld)
  else()
    zephyr_linker_sources(SECTIONS shared_mem_m33.ld)
  endif()
endif()

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/soc/infineon/edge/pse84/linker_exclude_syslib.ld CACHE INTERNAL "")

zephyr_linker_sources_ifdef(CONFIG_CPU_CORTEX_M33 RWDATA rwdata.ld)
zephyr_linker_sources_ifdef(CONFIG_CPU_CORTEX_M55 RWDATA rwdata.ld)

zephyr_linker_sources_ifdef(CONFIG_CPU_CORTEX_M55 ITCM_SECTION itcm_mem.ld)
zephyr_linker_sources_ifdef(CONFIG_CPU_CORTEX_M33 RAMFUNC_SECTION ramfunc_mem.ld)
