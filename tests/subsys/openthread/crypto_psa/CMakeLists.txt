# Copyright (c) 2026 The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(openthread_crypto_psa)

if(NOT ZEPHYR_OPENTHREAD_MODULE_DIR)
  message(FATAL_ERROR "OpenThread module directory not found")
endif()

# Build the platform implementation here. CONFIG_OPENTHREAD also builds and
# links the OpenThread stack, which this test does not run. The compile
# definitions select the same context ownership as
# CONFIG_OPENTHREAD_CRYPTO_PLATFORM_ALLOCS_CONTEXT with PSA key references.
target_sources(app PRIVATE
  src/main.c
  ${ZEPHYR_BASE}/modules/openthread/platform/crypto_psa.c
)
target_compile_definitions(app PRIVATE
  CONFIG_OPENTHREAD_CRYPTO_PLATFORM_ALLOCS_CONTEXT
  CONFIG_OPENTHREAD_PLATFORM_KEY_REF=1
)
target_include_directories(app PRIVATE
  ${ZEPHYR_OPENTHREAD_MODULE_DIR}/include
)
