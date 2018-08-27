# Copyright (c) 2018 Bobby Noelte <b0661n0e17e@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0

# Prepare generic unit test environment
#
# parameters:
# - INCLUDE is relative to zephyr base
# - SOURCES defaults to main.c
#
# target:
# - testbinary (default)

# for ztest
list(APPEND INCLUDE subsys)
# for code generation
set(template_file ${CMAKE_CURRENT_SOURCE_DIR}/main.c)
set(SOURCES "") # prevent default
include($ENV{ZEPHYR_BASE}/tests/unit/unittest.cmake)

# Fake zephyr environment to prepare the
# generation of include/generated/generated_dts_board.h from DTS
set(ZEPHYR_BASE $ENV{ZEPHYR_BASE})
set(PROJECT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(PROJECT_SOURCE_DIR ${ZEPHYR_BASE})
set(ARCH arm)
set(BOARD_FAMILY nucleo_f091rc)
set(BOARD nucleo_f091rc)
set(CONFIG_HAS_DTS True)
set(APPLICATION_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

include(${ZEPHYR_BASE}/cmake/extensions.cmake)

# Get the tools for DTS - python and DTC
find_package(PythonInterp 3.4)
include(${ZEPHYR_BASE}/cmake/host-tools.cmake)

# fake autconf.h generation
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/generated)
file(COPY autoconf.h DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/generated)

# Generate include/generated/generated_dts_board.h from DTS
set(AUTOCONF_H ${CMAKE_CURRENT_SOURCE_DIR}/autoconf.h)
set(DTS_SOURCE ${PROJECT_SOURCE_DIR}/boards/arm/nucleo_f091rc/nucleo_f091rc.dts)
include(${ZEPHYR_BASE}/cmake/dts.cmake)

# fake syscall generation for header files that are needed
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/syscalls)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/include/syscalls/pinctrl.h "/* test only */")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/include/syscalls/gpio.h "/* test only */")

# Prepare for inline code generation
target_sources_codegen(testbinary ${template_file})

# Make test aware of
# - dts bindings header files
# - generated include file
target_include_directories(testbinary PRIVATE
                            ${DTS_BINDINGS_DIR}
                            ${CMAKE_CURRENT_SOURCE_DIR}
                            ${CMAKE_CURRENT_BINARY_DIR}/include
                            ${CMAKE_CURRENT_BINARY_DIR}/include/generated
                            )

project(none) # switches PROJECT_SOURCE_DIR and PROJECT_BINARY_DIR
