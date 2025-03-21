# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(MURISCV_NN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../modules/lib/muriscv-nn)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(muriscv-nn)



add_subdirectory(${MURISCV_NN_DIR} muriscv-nn_build)
target_compile_options(muriscvnn PRIVATE -march=rv64gcv -mabi=lp64d -DUSE_VEXT=1)
# add_compile_definitions(USE_VEXT=1 -DRISCV_ARCH=rv64gcv -DRISCV_ABI=lp64d)



include_directories(${MURISCV_NN_DIR}/Include)
target_link_libraries(app PRIVATE muriscvnn)
target_sources(app PRIVATE src/main.c)
