# Copyright (c) 2021 Nordic Semiconductor
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

# This adds the primary application to the build.
ExternalZephyrProject_Add(
  APPLICATION ${app_name}
  SOURCE_DIR ${APP_DIR}
  MAIN_APP
)

# This allows for board and app specific images to be included.
include(${BOARD_DIR}/sysbuild.cmake OPTIONAL)
include(${APP_DIR}/sysbuild.cmake OPTIONAL)
