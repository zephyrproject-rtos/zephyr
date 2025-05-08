# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

set(MCUX_SDK_NG_DIR ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk-ng)
# SdkRootDirPath is used by MCUX SDK NG CMake files.
set(SdkRootDirPath ${MCUX_SDK_NG_DIR})

if(CONFIG_CPU_CORTEX_A)
  set(ACoreCmsisDirPath ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/CMSIS)
endif()

# Functions for MCUX SDK cmake files
include(${MCUX_SDK_NG_DIR}/cmake/extension/logging.cmake)
include(${MCUX_SDK_NG_DIR}/cmake/extension/function.cmake)
include(${MCUX_SDK_NG_DIR}/cmake/extension/basic_settings_lite.cmake)

function(set_variable_ifdef feature_toggle variable)
  if(${${feature_toggle}})
    set(${variable} ON PARENT_SCOPE)
  endif()
endfunction()
