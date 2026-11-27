# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Defines "psa_crypto_interface", the interface library through which the PSA
# Crypto headers reach the build. The PSA Crypto provider adds the include
# directories of the headers to it.

# The order in which modules are processed is not fixed, so a provider outside
# of the Mbed TLS module must include this file before it uses the library.

include_guard(GLOBAL)

zephyr_interface_library_named(psa_crypto_interface)

# Public Zephyr headers include <psa/crypto.h>, so any source file may need it.
zephyr_link_libraries(psa_crypto_interface)

target_compile_definitions(psa_crypto_interface INTERFACE
  TF_PSA_CRYPTO_CONFIG_FILE="${CONFIG_TF_PSA_CRYPTO_CONFIG_FILE}"
)
target_include_directories(psa_crypto_interface INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/configs
)
