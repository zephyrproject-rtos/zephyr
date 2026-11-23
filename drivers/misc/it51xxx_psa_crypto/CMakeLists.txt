# Copyright (c) 2026 ITE Corporation.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

zephyr_library_sources(
  it51xxx_crypto.c
)

zephyr_library_sources_ifdef(CONFIG_IT51XXX_PSA_CRYPTO_SHA it51xxx_crypto_sha.c)
zephyr_library_sources_ifdef(CONFIG_IT51XXX_PSA_CRYPTO_RSA it51xxx_crypto_rsa.c)
