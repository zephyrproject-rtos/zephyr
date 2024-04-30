# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

project(bluetooth_ascs)
find_package(Zephyr COMPONENTS unittest HINTS $ENV{ZEPHYR_BASE})

add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/audio/cap_commander/uut uut)

target_link_libraries(testbinary PRIVATE uut)

target_include_directories(testbinary PRIVATE include)

target_sources(testbinary
  PRIVATE
    ${ZEPHYR_BASE}/subsys/bluetooth/host/uuid.c
    src/main.c
    src/test_common.c
    src/test_vcp.c
    src/test_micp.c
)
