# Copyright (c) 2025 Croxel Inc.
# Copyright (c) 2025 CogniPilot Foundation
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_AFBR_LIB)

  zephyr_library()

  set(HAL_AFBR_DIR ${ZEPHYR_CURRENT_MODULE_DIR})

  zephyr_include_directories(
    ${HAL_AFBR_DIR}/AFBR-S50/Include
  )

  zephyr_library_sources(
    platform_irq.c
    platform_print.c
    platform_s2pi.c
    platform_timer.c
    platform_malloc.c
    platform_nvm.c
    platform_misc.c
  )

  add_library(afbr_sdk_lib STATIC IMPORTED GLOBAL)

  if(CONFIG_CPU_CORTEX_M0)
    set_target_properties(afbr_sdk_lib PROPERTIES IMPORTED_LOCATION
      ${HAL_AFBR_DIR}/zephyr/blobs/AFBR-S50/Lib/libafbrs50_m0.a
    )
  elseif(CONFIG_CPU_CORTEX_M3)
    set_target_properties(afbr_sdk_lib PROPERTIES IMPORTED_LOCATION
      ${HAL_AFBR_DIR}/zephyr/blobs/AFBR-S50/Lib/libafbrs50_m3.a
    )
  elseif(CONFIG_CPU_CORTEX_M33 AND CONFIG_FPU)
    set_target_properties(afbr_sdk_lib PROPERTIES IMPORTED_LOCATION
      ${HAL_AFBR_DIR}/zephyr/blobs/AFBR-S50/Lib/libafbrs50_m33_fpu.a
    )
  elseif(CONFIG_CPU_CORTEX_M33)
    set_target_properties(afbr_sdk_lib PROPERTIES IMPORTED_LOCATION
      ${HAL_AFBR_DIR}/zephyr/blobs/AFBR-S50/Lib/libafbrs50_m33.a
    )
  elseif(CONFIG_CPU_CORTEX_M4 AND CONFIG_FPU)
    set_target_properties(afbr_sdk_lib PROPERTIES IMPORTED_LOCATION
      ${HAL_AFBR_DIR}/zephyr/blobs/AFBR-S50/Lib/libafbrs50_m4_fpu.a
    )
  elseif(CONFIG_CPU_CORTEX_M4)
    set_target_properties(afbr_sdk_lib PROPERTIES IMPORTED_LOCATION
      ${HAL_AFBR_DIR}/zephyr/blobs/AFBR-S50/Lib/libafbrs50_m4.a
    )
  else()
    message(FATAL_ERROR "Unsupported CPU type")
  endif()

  zephyr_link_libraries(afbr_sdk_lib)

endif() # CONFIG_AFBR_LIB
