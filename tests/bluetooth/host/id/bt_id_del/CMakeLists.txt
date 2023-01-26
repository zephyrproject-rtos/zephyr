# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

project(bt_id_del)

# Suppress the format-zero-length error if GNUC is used
if (CMAKE_C_COMPILER_ID MATCHES "GNU")
add_compile_options(-Wno-error=format-zero-length -Wno-format-zero-length)
endif()

include_directories(BEFORE
    $ENV{ZEPHYR_BASE}/tests/bluetooth/host/id/mocks
)

find_package(Zephyr COMPONENTS unittest HINTS $ENV{ZEPHYR_BASE})

add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/host host_mocks)
add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/host/id/mocks mocks)

target_link_libraries(testbinary PRIVATE mocks host_mocks)

target_sources(testbinary
    PRIVATE
    src/main.c
    src/test_suite_invalid_inputs.c

    ${ZEPHYR_BASE}/subsys/bluetooth/host/id.c
    ${ZEPHYR_BASE}/subsys/bluetooth/common/addr.c
    ${ZEPHYR_BASE}/subsys/logging/log_minimal.c
    ${ZEPHYR_BASE}/subsys/bluetooth/common/bt_str.c
    ${ZEPHYR_BASE}/subsys/bluetooth/host/uuid.c
)
