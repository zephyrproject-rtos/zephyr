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

# Note: Difference between `core_id` and `core_id_suffix_name` in MCUX SDK NG.
#
# MCUX SDK NG uses `core_id` to distinguish which core currently is running on.
#
# In most of the time, `core_id` and `core_id_suffix_name` are the same, but
# there are some exceptions, for example: RT595 FUSIONF1.
# `core_id` is used for the core's folder name in device folder, like:
# "mcux-sdk-ng/devices/RT/RT500/MIMXRT595S/fusionf1", here `core_id` is fusionf1.
# `core_id_suffix_name` is used for the device files and macro suffix, such as:
# file "system_MIMXRT595S_dsp.h", macro `CPU_MIMXRT595SFAWC_dsp`, here
# `core_id_suffix_name` is dsp.
#
# MCUX SDK NG needs `core_id` as input, it defines `core_id_suffix_name` based on
# `core_id` in file "mcux-sdk-ng/devices/RT/RT500/MIMXRT595S/<core_id>".
#
# Zephyr provides `MCUX_CORE_SUFFIX` to distinguish the core, it is actaully the
# `core_id_suffix_name` in MCUX SDK NG, here convert it to `core_id`, then pass
# it to MCUX SDK NG.
if(DEFINED CONFIG_MCUX_CORE_SUFFIX)
  if (CONFIG_SOC_MIMXRT595S_F1)
    set(core_id "fusionf1")
  elseif (CONFIG_SOC_MIMXRT685S_HIFI4)
    set(core_id "hifi4")
  else()
    string (REGEX REPLACE "^_" "" core_id "${CONFIG_MCUX_CORE_SUFFIX}")
  endif()
endif()

if(CONFIG_SOC_SERIES_IMXRT10XX OR CONFIG_SOC_SERIES_IMXRT11XX)
  set(CONFIG_MCUX_COMPONENT_device.boot_header ON)
endif()

if(NOT (CONFIG_SOC_MIMX94398_M33 OR CONFIG_SOC_MIMX94398_M7_0 OR CONFIG_SOC_MIMX94398_M7_1))
  set(CONFIG_MCUX_COMPONENT_device.system ON)
endif()
set(CONFIG_MCUX_COMPONENT_device.CMSIS ON)
if(NOT CONFIG_CLOCK_CONTROL_ARM_SCMI)
  set(CONFIG_MCUX_COMPONENT_driver.clock ON)
endif()

# Exclude fsl_power.c for DSP domains
if((CONFIG_ARM) AND (NOT CONFIG_CLOCK_CONTROL_ARM_SCMI))
  set(CONFIG_MCUX_COMPONENT_driver.power ON)
endif()

if(NOT CONFIG_CPU_CORTEX_A)
  set(CONFIG_MCUX_COMPONENT_driver.reset ON)
  set(CONFIG_MCUX_COMPONENT_driver.memory ON)
  set(CONFIG_MCUX_COMPONENT_driver.sentinel ON)
endif()

# Include fsl_dsp.c for ARM domains (applicable to i.MX RTxxx devices)
if(CONFIG_ARM)
  set(CONFIG_MCUX_COMPONENT_driver.dsp ON)
endif()

# load device variables
include(${mcux_device_folder}/variable.cmake)

# Define CPU macro, like: CPU_MIMXRT595SFAWC_dsp.
# Variable `core_id_suffix_name` is from file ${mcux_device_folder}/variable.cmake
zephyr_compile_definitions("CPU_${CONFIG_SOC_PART_NUMBER}${core_id_suffix_name}")

# Definitions to load device drivers, like: CPU_MIMXRT595SFAWC_dsp.
set(CONFIG_MCUX_HW_DEVICE_CORE "${MCUX_DEVICE}${core_id_suffix_name}")

# Necessary values to load right SDK NG cmake files
# CONFIG_MCUX_HW_CORE
# CONFIG_MCUX_HW_FPU_TYPE
#
# They are used by the files like:
#   zephyr/modules/hal/nxp/mcux/mcux-sdk-ng/devices/arm/shared.cmake
#   zephyr/modules/hal/nxp/mcux/mcux-sdk-ng/devices/xtensa/shared.cmake
if (CONFIG_CPU_CORTEX_M0PLUS)
  set(CONFIG_MCUX_HW_CORE cm0p)
elseif (CONFIG_CPU_CORTEX_M3)
  set(CONFIG_MCUX_HW_CORE cm3)
elseif (CONFIG_CPU_CORTEX_M33)
  set(CONFIG_MCUX_HW_CORE cm33)
elseif (CONFIG_CPU_CORTEX_M4)
  if (CONFIG_CPU_HAS_FPU)
    set(CONFIG_MCUX_HW_CORE cm4f)
  else()
    set(CONFIG_MCUX_HW_CORE cm4)
  endif()
elseif (CONFIG_CPU_CORTEX_M7)
  set(CONFIG_MCUX_HW_CORE cm7f)
elseif (CONFIG_XTENSA)
  set(CONFIG_MCUX_HW_CORE dsp)
endif()

if (CONFIG_CPU_HAS_FPU)
  if (CONFIG_CPU_CORTEX_M33 OR CONFIG_CPU_CORTEX_M7)
    if (CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION)
      set(CONFIG_MCUX_HW_FPU_TYPE fpv5_dp)
    else()
      set(CONFIG_MCUX_HW_FPU_TYPE fpv5_sp)
    endif()
  elseif (CONFIG_CPU_CORTEX_M4)
    set(CONFIG_MCUX_HW_FPU_TYPE fpv4_sp)
  endif()
else()
  set(CONFIG_MCUX_HW_FPU_TYPE no_fpu)
endif()

# Load device files
mcux_add_cmakelists(${mcux_device_folder})

# Workaround for fsl_flexspi_nor_boot link error, remove the one in SDK, use the Zephyr file.
if(CONFIG_MCUX_COMPONENT_device.boot_header)

  get_target_property(MCUXSDK_SOURCES ${MCUX_SDK_PROJECT_NAME} SOURCES)
  list(FILTER MCUXSDK_SOURCES INCLUDE REGEX ".*fsl_flexspi_nor_boot\.c$")

  if(NOT MCUXSDK_SOURCES STREQUAL "")
    file(RELATIVE_PATH MCUXSDK_SOURCES ${SdkRootDirPath} ${MCUXSDK_SOURCES})
    mcux_project_remove_source(
      BASE_PATH ${SdkRootDirPath}
      SOURCES ${MCUXSDK_SOURCES}
    )
  endif()

endif()
