# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

# Expose the driver header include path, so that the shim driver can use them.
get_target_property(MCUXSDK_INCLUDE_DIRS ${MCUX_SDK_PROJECT_NAME} INTERFACE_INCLUDE_DIRECTORIES)
if(NOT MCUXSDK_INCLUDE_DIRS STREQUAL MCUXSDK_INCLUDE_DIRS-NOTFOUND)
  zephyr_include_directories(${MCUXSDK_INCLUDE_DIRS})
endif()

get_target_property(MCUXSDK_COMPILE_DEFS ${MCUX_SDK_PROJECT_NAME} INTERFACE_COMPILE_DEFINITIONS)
if(NOT MCUXSDK_COMPILE_DEFS STREQUAL MCUXSDK_COMPILE_DEFS-NOTFOUND)
  zephyr_compile_definitions(${MCUXSDK_COMPILE_DEFS})
endif()
