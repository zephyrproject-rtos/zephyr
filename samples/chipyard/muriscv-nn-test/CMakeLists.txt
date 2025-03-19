# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcmodel=medany -mabi=lp64d -march=rv64gcv -DUSE_VEXT=1")
set(MURISCV_NN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../modules/lib/muriscv-nn)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(muriscv-nn)

add_subdirectory(${MURISCV_NN_DIR} muriscv-nn_build)

include_directories(${MURISCV_NN_DIR}/Include)
include_directories(${MURISCV_NN_DIR}/Tests/TestCases/Common)
target_link_libraries(app PRIVATE muriscvnn unity)
target_sources(app PRIVATE ${MURISCV_NN_DIR}/Tests/TestCases/test_muriscv_nn_fully_connected_s8/test_muriscv_nn_fully_connected_s8.c)
