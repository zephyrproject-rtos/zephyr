# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

set(SdkRootDirPath ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk-ng)

# Functions for MCUX SDK cmake files
include(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk-ng/cmake/extension/logging.cmake)
include(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk-ng/cmake/extension/function.cmake)
include(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk-ng/cmake/extension/basic_settings_lite.cmake)

add_subdirectory(components)
add_subdirectory(device)
add_subdirectory(drivers)
add_subdirectory(middleware)

# Workaround:
# MCUXSDK Kconfig symbols may use dot, this symbols should be removed from autoconf.h
# and configs.c
if(NOT CONFIG_SOC_SDKNG_UNSUPPORTED)
  file(READ ${AUTOCONF_H} FILE_CONTENTS)
  string(REGEX REPLACE "#define CONFIG_MCUX_[A-Za-z|_|0-9]*\\.[^\n]+\n" "" STRIPPED "${FILE_CONTENTS}")
  file(WRITE ${AUTOCONF_H} "${STRIPPED}")

  file(READ ${CONFIGS_C} FILE_CONTENTS)
  string(REGEX REPLACE "#define CONFIG_MCUX_[A-Za-z|_|0-9]*\\.[^\n]+\n" "" STRIPPED "${FILE_CONTENTS}")
  file(WRITE ${CONFIGS_C} "${STRIPPED}")
endif()

# Expose the driver header include path, so that the shim driver can use them.
get_target_property(MCUXSDK_INCLUDE_DIRS ${MCUX_SDK_PROJECT_NAME} INTERFACE_INCLUDE_DIRECTORIES)
if (NOT MCUXSDK_INCLUDE_DIRS STREQUAL MCUXSDK_INCLUDE_DIRS-NOTFOUND)
  zephyr_include_directories(${MCUXSDK_INCLUDE_DIRS})
endif()

get_target_property(MCUXSDK_COMPILE_DEFS ${MCUX_SDK_PROJECT_NAME} INTERFACE_COMPILE_DEFINITIONS)
if (NOT MCUXSDK_COMPILE_DEFS STREQUAL MCUXSDK_COMPILE_DEFS-NOTFOUND)
  zephyr_compile_definitions(${MCUXSDK_COMPILE_DEFS})
endif()

# Workaround for fsl_flexspi_nor_boot link error, remove the one in SDK, use the Zephyr file.
if (CONFIG_MCUX_COMPONENT_device.boot_header)

get_target_property(MCUXSDK_SOURCES ${MCUX_SDK_PROJECT_NAME} SOURCES)
list(FILTER MCUXSDK_SOURCES INCLUDE REGEX ".*fsl_flexspi_nor_boot\.c$")

if (NOT MCUXSDK_SOURCES STREQUAL "")
  file(RELATIVE_PATH MCUXSDK_SOURCES ${SdkRootDirPath} ${MCUXSDK_SOURCES})
  mcux_project_remove_source(
    BASE_PATH ${SdkRootDirPath}
    SOURCES ${MCUXSDK_SOURCES}
  )
endif()

endif()

#specific operation to shared drivers
if((DEFINED CONFIG_FLASH_MCUX_FLEXSPI_XIP) AND (DEFINED CONFIG_FLASH))
  zephyr_code_relocate(FILES ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk-ng/drivers/flexspi/fsl_flexspi.c
    LOCATION ${CONFIG_FLASH_MCUX_FLEXSPI_XIP_MEM}_TEXT)
endif()

# note: if FSL_IRQSTEER_ENABLE_MASTER_INT is not
# defined then it will automatically be defined
# and set to 1 via fsl_irqsteer.h
zephyr_library_compile_definitions_ifdef(
  CONFIG_NXP_IRQSTEER
  FSL_IRQSTEER_ENABLE_MASTER_INT=0
)
