# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

# Use host ELF objects with the same .obj suffix as a Zephyr build.
set(CMAKE_SYSTEM_NAME Generic)
project(relocation_fixture C)

set(APPLICATION_BINARY_DIR ${PROJECT_BINARY_DIR})
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/include/generated)
add_library(linker INTERFACE)
add_library(compiler INTERFACE)
add_library(zephyr_interface INTERFACE)

configure_file(relocated.c relocated.c COPYONLY)
set(relocated_source ${PROJECT_BINARY_DIR}/relocated.c)
add_library(app STATIC ${relocated_source})
target_compile_options(app PRIVATE -ffunction-sections -fdata-sections)
add_library(kernel STATIC ${ZEPHYR_BASE}/misc/empty_file.c)

add_library(code_data_relocation_target INTERFACE)
list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/cmake/modules)
include(extensions)
include(${ZEPHYR_BASE}/cmake/linker/arcmwdt/target.cmake)
zephyr_code_relocate(FILES ${relocated_source} LOCATION RAM)
toolchain_ld_relocation()

add_custom_target(relocation_output
  DEPENDS
    ${MEM_RELOCATION_CODE}
    ${MEM_RELOCATION_LD}
)
