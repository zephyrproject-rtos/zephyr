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
set(Sysbuild_DIR ${CMAKE_CURRENT_LIST_DIR}/../sysbuild-package/cmake)

project(sysbuild_toplevel LANGUAGES)

if(EXISTS ${APP_DIR}/sysbuild/CMakeLists.txt)
  add_subdirectory(${APP_DIR}/sysbuild sysbuild/application)
else()
  add_subdirectory(template sysbuild/application)
endif()
