# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mbedtls)

set(output_file ${PROJECT_BINARY_DIR}/mbedtls-check.timestamp)

add_custom_command(
    COMMENT "Check Mbed TLS auto-generated files"
    COMMAND
        ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/modules/mbedtls/create_psa_files.py --check
    OUTPUT
        ${output_file}
)

add_custom_target(check_mbedtls_auto_generated_files ALL DEPENDS ${output_file})

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
