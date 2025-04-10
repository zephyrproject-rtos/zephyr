# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

set(MCUX_SDK_NG_DIR ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk-ng)
# SdkRootDirPath is used by MCUX SDK NG CMake files.
set(SdkRootDirPath ${MCUX_SDK_NG_DIR})

# Functions for MCUX SDK cmake files
include(${MCUX_SDK_NG_DIR}/cmake/extension/logging.cmake)
include(${MCUX_SDK_NG_DIR}/cmake/extension/function.cmake)
include(${MCUX_SDK_NG_DIR}/cmake/extension/basic_settings_lite.cmake)

function(set_variable_ifdef feature_toggle variable)
  if(${${feature_toggle}})
    set(${variable} ON PARENT_SCOPE)
  endif()
endfunction()

#specific operation to shared drivers
if((DEFINED CONFIG_FLASH_MCUX_FLEXSPI_XIP) AND (DEFINED CONFIG_FLASH))
  zephyr_code_relocate(FILES ${MCUX_SDK_NG_DIR}/drivers/flexspi/fsl_flexspi.c
    LOCATION ${CONFIG_FLASH_MCUX_FLEXSPI_XIP_MEM}_TEXT)
endif()

if(CONFIG_NXP_RF_IMU AND CONFIG_SOC_SERIES_MCXW)
  zephyr_compile_definitions(HAL_RPMSG_SELECT_ROLE=0U)
endif()

if((${MCUX_DEVICE} MATCHES "RW61") AND (NOT DEFINED CONFIG_MINIMAL_LIBC))
  # Whenever building for RW61x without minimal LIBC, use optimized memcpy.
  # This will avoid issues with unaligned access to peripheral RAM regions
  # caused by the memcpy implementation in newlib
  zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk-ng/components/misc_utilities/fsl_memcpy.S)
endif()

include(middleware/middleware.cmake)
include(components/components.cmake)
include(drivers/drivers.cmake)
include(device/device.cmake)

# Expose the driver header include path, so that the shim driver can use them.
get_target_property(MCUXSDK_INCLUDE_DIRS ${MCUX_SDK_PROJECT_NAME} INTERFACE_INCLUDE_DIRECTORIES)
if(NOT MCUXSDK_INCLUDE_DIRS STREQUAL MCUXSDK_INCLUDE_DIRS-NOTFOUND)
  zephyr_include_directories(${MCUXSDK_INCLUDE_DIRS})
endif()

get_target_property(MCUXSDK_COMPILE_DEFS ${MCUX_SDK_PROJECT_NAME} INTERFACE_COMPILE_DEFINITIONS)
if(NOT MCUXSDK_COMPILE_DEFS STREQUAL MCUXSDK_COMPILE_DEFS-NOTFOUND)
  zephyr_compile_definitions(${MCUXSDK_COMPILE_DEFS})
endif()
