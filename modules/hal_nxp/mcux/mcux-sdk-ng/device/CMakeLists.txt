# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

string(TOUPPER ${CONFIG_SOC} MCUX_DEVICE)

# Find the folder in mcux-sdk/devices that matches the device name
message(STATUS "Looking for device ${MCUX_DEVICE} in ${SdkRootDirPath}/devices/")

file(GLOB_RECURSE device_cmake_files ${SdkRootDirPath}/devices/*/CMakeLists.txt)
foreach(file ${device_cmake_files})
  get_filename_component(folder ${file} DIRECTORY)
  get_filename_component(folder_name ${folder} NAME)
  if(folder_name STREQUAL ${MCUX_DEVICE})
    message(STATUS "Found device folder: ${folder}")
    set(mcux_device_folder ${folder})
    break()
  endif()
endforeach()

if(NOT mcux_device_folder)
  message(FATAL_ERROR "Device ${MCUX_DEVICE} not found in ${SdkRootDirPath}/devices/")
endif()

if (DEFINED CONFIG_MCUX_CORE_SUFFIX)
  string (REGEX REPLACE "^_" "" core_id "${CONFIG_MCUX_CORE_SUFFIX}")
endif()

# Define CPU macro
zephyr_compile_definitions("CPU_${CONFIG_SOC_PART_NUMBER}${CONFIG_MCUX_CORE_SUFFIX}")

# load device variables
include(${mcux_device_folder}/variable.cmake)

# Load device files
mcux_add_cmakelists(${mcux_device_folder})

zephyr_compile_definitions_ifdef(
  CONFIG_SOC_MIMX9596
  FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL=1
)

zephyr_compile_definitions_ifdef(
  CONFIG_SOC_MIMX9596
  FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL=1
)
