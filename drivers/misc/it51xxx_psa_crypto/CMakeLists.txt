# Copyright (c) 2026 ITE Corporation.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

message(STATUS "The it51xxx psa crypto driver is included")

set(tfpsacrypto_target "${TF_PSA_CRYPTO_TARGET_PREFIX}tfpsacrypto")

cmake_language(EVAL CODE "
  cmake_language(DEFER DIRECTORY [[${ZEPHYR_BASE}]]
    CALL target_compile_definitions [[${tfpsacrypto_target}]] PRIVATE
      PSA_CRYPTO_ACCELERATOR_DRIVER_PRESENT
      PSA_CRYPTO_DRIVER_PRESENT
      PSA_CRYPTO_DRIVER_ITE_IT51XXX
  )
")

if(CONFIG_IT51XXX_PSA_CRYPTO_SHA)
  cmake_language(EVAL CODE "
    cmake_language(DEFER DIRECTORY [[${ZEPHYR_BASE}]]
      CALL target_compile_definitions [[${tfpsacrypto_target}]] PRIVATE
        PSA_CRYPTO_DRIVER_ITE_IT51XXX_SHA
    )
  ")
endif()

zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

zephyr_library_include_directories(
  "$<TARGET_PROPERTY:${tfpsacrypto_target},INCLUDE_DIRECTORIES>"
)

zephyr_library_sources(
  it51xxx_crypto.c
)

zephyr_library_sources_ifdef(CONFIG_IT51XXX_PSA_CRYPTO_SHA it51xxx_crypto_sha.c)
