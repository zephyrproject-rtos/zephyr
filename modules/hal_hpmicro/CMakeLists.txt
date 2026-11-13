# Copyright (c) 2026 Cody Gu <gujiaqi@iscas.ac.cn>
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_FAMILY_HPMICRO)

  set(HPM_SDK_DIR "${ZEPHYR_HAL_HPMICRO_MODULE_DIR}/hpm_sdk")
  set(HPM_SDK_BASE "${HPM_SDK_DIR}")
  include(${HPM_SDK_DIR}/cmake/version.cmake)
  include(${HPM_SDK_DIR}/cmake/cmake-ext.cmake)
  include(${HPM_SDK_DIR}/cmake/ide/segger.cmake)
  include(${HPM_SDK_DIR}/cmake/ide/iar.cmake)

  if(CONFIG_XIP)
    set(INCLUDE_BOOTHEADER 1)
  endif()

  set(HPM_SDK_BINARY_DIR ${CMAKE_BINARY_DIR}/hpm_sdk)
  set(HPM_BOARD_DIR ${CONFIG_BOARD})
  string(TOUPPER ${CONFIG_SOC} HPM_SOC_TOUPPER)
  set(HPM_SOC ${HPM_SOC_TOUPPER})
  string(TOUPPER ${CONFIG_SOC_SERIES} HPM_SOC_SERIES_TOUPPER)
  set(HPM_SOC_SERIES ${HPM_SOC_SERIES_TOUPPER})
  set(TOOLCHAIN_VARIANT "zephyr")
  set(ENV{GNURISCV_TOOLCHAIN_PATH} ${SYSROOT_DIR})
  set(HPM_SDK_LIB_ITF zephyr_interface)
  set(HPM_SDK_GCC_LIB_ITF zephyr_interface)
  set(HPM_SDK_NDSGCC_LIB_ITF hpm_sdk_ndsgcc_lib_itf)
  add_library(${HPM_SDK_NDSGCC_LIB_ITF} INTERFACE)
  set(HPM_SDK_ZCC_LIB_ITF hpm_sdk_zcc_lib_itf)
  add_library(${HPM_SDK_ZCC_LIB_ITF} INTERFACE)
  set(USE_CUSTOM_STARTUP 1)
  set(USE_CUSTOM_LINKER 1)
  set(HPM_SOC_DISABLE_B_EXT 1)

  zephyr_library_named(hpm_sdk)
  zephyr_library_sources(${CMAKE_CURRENT_LIST_DIR}/shim/board_compat.c)
  set(HPM_SDK_LIB hpm_sdk)
  set(HPM_SDK_GCC_LIB hpm_sdk)
  set(HPM_SDK_GCC_STARTUP_LIB hpm_sdk)

  set(HPM_SOC_DIR "${HPM_SDK_DIR}/soc/${HPM_SOC_SERIES}/${HPM_SOC}")
  set(HPM_SOC_MODULE_LIST "${HPM_SOC_DIR}/soc_modules.list")

  if(NOT EXISTS "${HPM_SOC_MODULE_LIST}")
    message(FATAL_ERROR "HPM SDK does not support ${HPM_SOC_SERIES}/${HPM_SOC}")
  endif()

  # HPMSOC_HAS_HPMSDK_* is both a compile-time capability macro consumed by SDK
  # headers and the CMake switch used by sdk_src_ifdef() to select HAL sources.
  # Keep the capability definitions for the whole SoC, but only define the CMake
  # variables for source selection when a Zephyr consumer selected that HAL.
  file(STRINGS "${HPM_SOC_MODULE_LIST}" HPM_SDK_MODULES REGEX "^HPMSOC_")
  set(HPM_SDK_CAPABILITIES)
  foreach(module IN LISTS HPM_SDK_MODULES)
    string(REGEX REPLACE "=.*$" "" module_name "${module}")
    string(REGEX REPLACE "^[^=]*=" "" module_value "${module}")
    list(APPEND HPM_SDK_CAPABILITIES "${module_name}")
    if(module_value STREQUAL "y")
      sdk_compile_definitions("-D${module_name}=y")
    endif()
  endforeach()

  if(CONFIG_HPM_SDK_BUILD_ALL_DRIVERS)
    foreach(module_name IN LISTS HPM_SDK_CAPABILITIES)
      set(${module_name} y)
    endforeach()
  else()
    file(STRINGS "${CMAKE_BINARY_DIR}/zephyr/.config" HPM_SDK_CONFIG_LINES
         REGEX "^CONFIG_USE_HPMSDK_.+=y$"
    )
    foreach(line IN LISTS HPM_SDK_CONFIG_LINES)
      string(REGEX REPLACE "CONFIG_USE_HPMSDK_([A-Za-z0-9_]+)=y" "\\1" use_name "${line}")
      set(module_name "HPMSOC_HAS_HPMSDK_${use_name}")
      list(FIND HPM_SDK_CAPABILITIES "${module_name}" module_index)
      if(module_index EQUAL -1)
        message(FATAL_ERROR
                "CONFIG_USE_HPMSDK_${use_name} has no matching SDK module for ${HPM_SOC}"
        )
      endif()
      set(${module_name} y)
    endforeach()
  endif()

  zephyr_include_directories(${HPM_SDK_DIR}/drivers/inc)
  zephyr_include_directories(${HPM_SDK_DIR}/arch)
  zephyr_include_directories(${HPM_SOC_DIR})
  zephyr_include_directories(${HPM_SDK_DIR}/soc/${HPM_SOC_SERIES}/ip)
  zephyr_include_directories(${HPM_SDK_DIR}/components/plb/filter)
  zephyr_include_directories(${HPM_SDK_DIR}/components/plb/qei_encoder)
  zephyr_include_directories(${CMAKE_CURRENT_LIST_DIR}/include)

  add_subdirectory(${HPM_SDK_DIR}/arch ${HPM_SDK_BINARY_DIR}/arch)
  add_subdirectory(${HPM_SDK_DIR}/soc ${HPM_SDK_BINARY_DIR}/soc)
  add_subdirectory(${HPM_SDK_DIR}/drivers ${HPM_SDK_BINARY_DIR}/drivers)

  # The pinned vendor QEIV2 driver promotes float to double in two places.
  # Suppress only that warning class for this source, whether it is selected by
  # the QEIV2 driver or by the all-HAL regression; all other SDK sources retain
  # Zephyr's warnings-as-errors policy.
  set_source_files_properties("${HPM_SDK_DIR}/drivers/src/hpm_qeiv2_drv.c"
                              PROPERTIES COMPILE_OPTIONS "-Wno-double-promotion"
  )

endif()
