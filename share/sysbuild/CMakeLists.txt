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

if(NOT DEFINED APPLICATION_BINARY_DIR)
  set(APPLICATION_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "Application Binary Directory")
endif()

if(EXISTS ${APP_DIR}/sysbuild/CMakeLists.txt)
  add_subdirectory(${APP_DIR}/sysbuild _sysbuild)
else()
  add_subdirectory(template _sysbuild)
endif()

build_info(sysbuild VALUE true)
build_info(application source-dir VALUE ${CMAKE_CURRENT_SOURCE_DIR})
yaml_context(EXISTS NAME build_info result)
if(result)
  yaml_save(NAME build_info)
endif()
