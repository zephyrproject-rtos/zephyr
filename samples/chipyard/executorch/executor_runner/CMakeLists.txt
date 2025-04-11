# Copyright 2023-2024 Arm Limited and/or its affiliates.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

cmake_minimum_required(VERSION 3.20)


add_compile_definitions(_POSIX_C_SOURCE=200809L)
add_compile_definitions(__POSIX_VISIBLE=200809L)

find_program(FLATC_EXECUTABLE flatc)
set(ET_DIR_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../../third-party/executorch" CACHE PATH "Path to ExecuTorch dir")
set(ET_BUILD_DIR_PATH "${ET_DIR_PATH}/cmake-out" CACHE PATH "Path to ExecuTorch build dir")
set(ET_INCLUDE_PATH "${ET_DIR_PATH}/.." CACHE PATH "Path to ExecuTorch headers")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(riscv_executor_runner)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(EXECUTORCH_BUILD_EXECUTOR_RUNNER OFF CACHE BOOL "")
set(EXECUTORCH_BUILD_EXTENSION_RUNNER_UTIL ON CACHE BOOL "")
set(EXECUTORCH_BUILD_KERNELS_QUANTIZED ON CACHE BOOL "")
set(EXECUTORCH_ENABLE_LOGGING ON CACHE BOOL "")
set(EXECUTORCH_BUILD_CPUINFO OFF CACHE BOOL "")
set(EXECUTORCH_BUILD_PTHREADPOOL OFF CACHE BOOL "")
set(EXECUTORCH_BUILD_GFLAGS OFF CACHE BOOL "")

set(EXECUTORCH_BUILD_HOST_TARGETS OFF CACHE BOOL "")

set(EXECUTORCH_BUILD_XNNPACK ON CACHE BOOL "")
set(XNNPACK_ENABLE_RISCV_GEMMINI ON CACHE BOOL "Build XNNPACK with RISC-V Gemmini micro-kernels")
set(XNNPACK_ENABLE_RISCV_VECTOR ON CACHE BOOL "Build XNNPACK with RISC-V Vector micro-kernels")
set(XNNPACK_BUILD_TESTS OFF CACHE BOOL "Build XNNPack unit tests")
set(XNNPACK_BUILD_BENCHMARKS OFF CACHE BOOL "Build XNNPack benchmarks")
set(FLATCC_RTONLY ON CACHE BOOL "enable build of runtime library of flatbuffers only")
# add_definitions(-DFLATBUFFERS_NO_ABSOLUTE_PATH_RESOLUTION)



get_filename_component(ET_BUILD_DIR_PATH ${ET_BUILD_DIR_PATH} REALPATH)
get_filename_component(ET_DIR_PATH ${ET_DIR_PATH} REALPATH)
get_filename_component(ET_INCLUDE_PATH ${ET_INCLUDE_PATH} REALPATH)

add_subdirectory(${ET_DIR_PATH} ${ET_BUILD_DIR_PATH})



add_library(compile_commands INTERFACE)
target_include_directories(compile_commands INTERFACE $<TARGET_PROPERTY:app,INTERFACE_INCLUDE_DIRECTORIES>)
target_compile_options(compile_commands INTERFACE $<TARGET_PROPERTY:app,INTERFACE_COMPILE_OPTIONS>)
target_compile_definitions(compile_commands INTERFACE $<TARGET_PROPERTY:app,INTERFACE_COMPILE_DEFINITIONS>)
add_dependencies(compile_commands zephyr_interface)
add_dependencies(compile_commands zephyr_property_target)


target_link_libraries(pthreadpool PUBLIC compile_commands)





target_link_libraries(app PRIVATE
  # "-Wl,--whole-archive" 
  extension_runner_util executorch executorch_core quantized_ops_lib portable_ops_lib quantized_kernels portable_kernels 
  xnnpack_backend XNNPACK microkernels-prod
  # "-Wl,--no-whole-archive"`
)

target_include_directories(app PRIVATE ${ET_INCLUDE_PATH} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(app PRIVATE riscv_executor_runner.cpp model_pte.c)
zephyr_linker_sources(DATA_SECTIONS  custom-sections.ld)

