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

# Propagate bootloader and signing settings from this system to the MCUboot and
# application image build systems.
if(SB_CONFIG_BOOTLOADER_MCUBOOT)
  set(${app_name}_CONFIG_BOOTLOADER_MCUBOOT y CACHE STRING
      "MCUBOOT is enabled as bootloader" FORCE
  )
  set(${app_name}_CONFIG_MCUBOOT_SIGNATURE_KEY_FILE
      \"${SB_CONFIG_BOOT_SIGNATURE_KEY_FILE}\" CACHE STRING
      "Signature key file for signing" FORCE
  )

  if(SB_CONFIG_BOOT_SIGNATURE_TYPE_NONE)
    set(${app_name}_CONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE y CACHE STRING
        "MCUBOOT is configured for checksum mode" FORCE
    )
  endif()

  # Set corresponding values in mcuboot
  set(mcuboot_CONFIG_BOOT_SIGNATURE_TYPE_${SB_CONFIG_SIGNATURE_TYPE} y CACHE STRING
      "MCUBOOT signature type" FORCE
  )
  set(mcuboot_CONFIG_BOOT_SIGNATURE_KEY_FILE
      \"${SB_CONFIG_BOOT_SIGNATURE_KEY_FILE}\" CACHE STRING
      "Signature key file for signing" FORCE
  )
else()
  set(${app_name}_CONFIG_BOOTLOADER_MCUBOOT n CACHE STRING
      "MCUBOOT is disabled as bootloader" FORCE
  )
endif()

# This adds the primary application to the build.
ExternalZephyrProject_Add(
  APPLICATION ${app_name}
  SOURCE_DIR ${APP_DIR}
  MAIN_APP
)
set(DEFAULT_IMAGE "${app_name}")

add_subdirectory(bootloader)

# This allows for board and app specific images to be included.
include(${BOARD_DIR}/sysbuild.cmake OPTIONAL)

# This allows image specific sysbuild.cmake to be processed.
set(i 0)
while(TRUE)
  get_property(IMAGES GLOBAL PROPERTY sysbuild_images)
  list(LENGTH IMAGES images_length)
  if(i GREATER_EQUAL ${images_length})
    break()
  endif()

  list(GET IMAGES ${i} image)
  list(FIND IMAGES ${image} j)
  if(NOT ${i} EQUAL ${j})
    message(FATAL_ERROR "Found multiple images named '${image}'. Image names must be unique.")
  endif()

  ExternalProject_Get_property(${image} SOURCE_DIR)
  include(${SOURCE_DIR}/sysbuild.cmake OPTIONAL)

  math(EXPR i "${i} + 1")
endwhile()

get_property(GROUPS GLOBAL PROPERTY sysbuild_groups)
sysbuild_groups_resolve(GROUPS ${GROUPS} IMAGES ${IMAGES})

sysbuild_images_order(IMAGES_CONFIGURATION_ORDER CONFIGURE IMAGES ${IMAGES} GROUPS ${GROUPS})
sysbuild_images_order(IMAGES_FLASHING_ORDER FLASH IMAGES ${IMAGES} GROUPS ${GROUPS})

sysbuild_module_call(PRE_CMAKE MODULES ${SYSBUILD_MODULE_NAMES} IMAGES ${IMAGES_CONFIGURATION_ORDER})
foreach(image ${IMAGES_CONFIGURATION_ORDER})
  ExternalZephyrProject_Cmake(APPLICATION ${image})
endforeach()
sysbuild_module_call(POST_CMAKE MODULES ${SYSBUILD_MODULE_NAMES} IMAGES ${IMAGES_CONFIGURATION_ORDER})

sysbuild_module_call(PRE_DOMAINS MODULES ${SYSBUILD_MODULE_NAMES} IMAGES ${IMAGES_FLASHING_ORDER})
include(cmake/domains.cmake)
sysbuild_module_call(POST_DOMAINS MODULES ${SYSBUILD_MODULE_NAMES} IMAGES ${IMAGES_FLASHING_ORDER})
