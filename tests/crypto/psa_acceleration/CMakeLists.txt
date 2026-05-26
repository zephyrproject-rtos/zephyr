# Copyright (c) 2026 Embeint Inc
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(psa_acceleration)

# The following libraries need to be able to find 'psa_external_integration_primitives.h'
#       tfpsacrypto
#       mbedTLS
#       External integration implementation
target_include_directories(tfpsacrypto INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
target_include_directories(mbedTLS INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

target_sources(app PRIVATE
        src/main.c
        # Standard functionality continues to work
        ../mbedtls_psa/src/main.c
)
