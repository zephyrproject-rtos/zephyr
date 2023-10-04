# Copyright (c) 2021-2023 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED APP_DIR)
  message(FATAL_ERROR "No main application specified")
endif()

# This will update the APP_DIR cache variable to PATH type and apply a comment.
# If APP_DIR is a relative path, then CMake will adjust to absolute path based
# on current working dir.
set(APP_DIR ${APP_DIR} CACHE PATH "Main Application Source Directory")

# Add sysbuild/cmake/modules to CMAKE_MODULE_PATH which allows us to integrate
# sysbuild CMake modules with general Zephyr CMake modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake/modules)
# List of Zephyr and sysbuild CMake modules we need for sysbuild.
# Note: sysbuild_kconfig will internally load kconfig CMake module.
set(zephyr_modules extensions sysbuild_extensions python west root zephyr_module boards shields sysbuild_kconfig)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE} COMPONENTS ${zephyr_modules})

project(sysbuild LANGUAGES)

# Global list of images enabled in this multi image build system.
set(IMAGES)

get_filename_component(APP_DIR  ${APP_DIR} ABSOLUTE)
get_filename_component(app_name ${APP_DIR} NAME)

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
    add_subdirectory(${SYSBUILD_CURRENT_CMAKE_DIR} ${CMAKE_BINARY_DIR}/modules/${SYSBUILD_CURRENT_MODULE_NAME})
  endif()
endforeach()
# Done processing modules, clear SYSBUILD_CURRENT_MODULE_DIR and SYSBUILD_CURRENT_CMAKE_DIR.
set(SYSBUILD_CURRENT_MODULE_DIR)
set(SYSBUILD_CURRENT_CMAKE_DIR)

# This adds the primary application to the build.
ExternalZephyrProject_Add(
  APPLICATION ${app_name}
  SOURCE_DIR ${APP_DIR}
  APP_TYPE MAIN
)
list(APPEND IMAGES "${app_name}")
set(DEFAULT_IMAGE "${app_name}")

add_subdirectory(bootloader)

# This allows for board and app specific images to be included.
include(${BOARD_DIR}/sysbuild.cmake OPTIONAL)

# This allows image specific sysbuild.cmake to be processed.
list(LENGTH IMAGES images_length)
while(NOT "${images_length}" EQUAL "${processed_length}")
  foreach(image ${IMAGES})
    if(NOT image IN_LIST images_sysbuild_processed)
      ExternalProject_Get_property(${image} SOURCE_DIR)
      include(${SOURCE_DIR}/sysbuild.cmake OPTIONAL)
      list(APPEND images_sysbuild_processed ${image})
    endif()
  endforeach()

  list(LENGTH IMAGES images_length)
  list(LENGTH images_sysbuild_processed processed_length_new)

  # Check for any duplicate entries in image names to prevent an infinite loop
  if("${processed_length_new}" EQUAL "${processed_length}")
    # Image length was different than processed length, but no new images are processed.
    message(FATAL_ERROR "A duplicate image name was provided, image names must be unique.")
  endif()
  set(processed_length ${processed_length_new})
endwhile()

sysbuild_module_call(PRE_CMAKE MODULES ${SYSBUILD_MODULE_NAMES} IMAGES ${IMAGES})
foreach(image ${IMAGES})
  include(image_config.cmake)
  ExternalZephyrProject_Cmake(APPLICATION ${image})
endforeach()
sysbuild_module_call(POST_CMAKE MODULES ${SYSBUILD_MODULE_NAMES} IMAGES ${IMAGES})

sysbuild_module_call(PRE_DOMAINS MODULES ${SYSBUILD_MODULE_NAMES} IMAGES ${IMAGES})
include(cmake/domains.cmake)
sysbuild_module_call(POST_DOMAINS MODULES ${SYSBUILD_MODULE_NAMES} IMAGES ${IMAGES})
