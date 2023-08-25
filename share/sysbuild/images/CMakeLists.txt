# Copyright (c) 2021-2023 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

# The primary application is the first image to be added to the build, so that
# it is available while processing the remaining images.
ExternalZephyrProject_Add(
  APPLICATION ${DEFAULT_IMAGE}
  SOURCE_DIR ${APP_DIR}
  APP_TYPE MAIN
)

# This allows for MCUboot to be included.
sysbuild_add_subdirectory(bootloader)

# Include zephyr modules generated sysbuild CMake file.
foreach(SYSBUILD_CURRENT_MODULE_NAME ${SYSBUILD_MODULE_NAMES})
  # Note the second, binary_dir parameter requires the added
  # subdirectory to have its own, local cmake target(s). If not then
  # this binary_dir is created but stays empty. Object files land in
  # the main binary dir instead.
  # https://cmake.org/pipermail/cmake/2019-June/069547.html
  zephyr_string(SANITIZE TOUPPER MODULE_NAME_UPPER ${SYSBUILD_CURRENT_MODULE_NAME})
  if(NOT ${SYSBUILD_${MODULE_NAME_UPPER}_CMAKE_DIR} STREQUAL "")
    set(SYSBUILD_CURRENT_MODULE_DIR ${SYSBUILD_${MODULE_NAME_UPPER}_MODULE_DIR})
    set(SYSBUILD_CURRENT_CMAKE_DIR ${SYSBUILD_${MODULE_NAME_UPPER}_CMAKE_DIR})
    sysbuild_add_subdirectory(${SYSBUILD_CURRENT_CMAKE_DIR}
                              ${CMAKE_BINARY_DIR}/modules/${SYSBUILD_CURRENT_MODULE_NAME})
  endif()
endforeach()
# Done processing modules, clear SYSBUILD_CURRENT_MODULE_DIR and SYSBUILD_CURRENT_CMAKE_DIR.
set(SYSBUILD_CURRENT_MODULE_DIR)
set(SYSBUILD_CURRENT_CMAKE_DIR)

# This allows for board specific images to be included.
sysbuild_add_subdirectory(boards)
