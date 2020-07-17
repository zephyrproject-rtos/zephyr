# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.8.2)

if (NOT BOARD)
  set (BOARD nucleo_f746zg)
endif ()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(wamr-simple)

enable_language (ASM)

add_definitions(-DWA_MALLOC=wasm_runtime_malloc)
add_definitions(-DWA_FREE=wasm_runtime_free)

set (WAMR_BUILD_PLATFORM "zephyr")

# Build as THUMB by default
# change to "ARM[sub]", "THUMB[sub]", "X86_32", "MIPS_32" or "XTENSA_32"
# if we want to support arm_32, x86, mips or xtensa
if (NOT DEFINED WAMR_BUILD_TARGET)
  set (WAMR_BUILD_TARGET "THUMBV7")
endif ()

if (NOT DEFINED WAMR_BUILD_INTERP)
  # Enable Interpreter by default
  set (WAMR_BUILD_INTERP 1)
endif ()

if (NOT DEFINED WAMR_BUILD_AOT)
  # Enable AOT by default.
  set (WAMR_BUILD_AOT 1)
endif ()

if (NOT DEFINED WAMR_BUILD_LIBC_BUILTIN)
  # Enable libc builtin support by default
  set (WAMR_BUILD_LIBC_BUILTIN 1)
endif ()

if (NOT DEFINED WAMR_BUILD_LIBC_WASI)
  # Disable libc wasi support by default
  set (WAMR_BUILD_LIBC_WASI 0)
endif ()

if (NOT DEFINED WAMR_BUILD_APP_FRAMEWORK)
  set (WAMR_BUILD_APP_FRAMEWORK 1)
endif ()

if (NOT DEFINED WAMR_BUILD_APP_LIST)
  set (WAMR_BUILD_APP_LIST WAMR_APP_BUILD_BASE
                           WAMR_APP_BUILD_SENSOR
                           WAMR_APP_BUILD_CONNECTION)
endif ()

if (NOT WAMR_ROOT_DIR)
  set (WAMR_ROOT_DIR
       ${CMAKE_CURRENT_SOURCE_DIR}/../../../../modules/lib/wasm-micro-runtime)
endif ()

include (${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)

target_sources(app PRIVATE
               ${WAMR_RUNTIME_LIB_SOURCE}
               src/iwasm_main.c src/main.c)
