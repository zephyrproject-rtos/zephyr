# Copyright (c) 2026 BayLibre SAS
#
# SPDX-License-Identifier: Apache-2.0

# Copy header files related to legacy crypto to the build folder in a path
# that does not contain "private" in the name. This allows legacy includes
# like "#include <mbedtls/ecp.h>" to still work. This is a temporary
# fix in order not to break external modules (ex: hostap) which are
# still referencing legacy includes. However these files are private now
# and all the users of legacy Mbed TLS should transition to PSA API as soon
# as possible!
if(CONFIG_MBEDTLS_DECLARE_PRIVATE_IDENTIFIERS)
  message(WARNING "
    Enabling CONFIG_MBEDTLS_DECLARE_PRIVATE_IDENTIFIERS is discouraged as it
    gives access to Mbed TLS crypto functions which are internal and may be removed
    or modified at any time. Please transition to the PSA Crypto API."
  )
  set(MBEDTLS_PRIVATE_INCLUDE_PATH "${ZEPHYR_TF_PSA_CRYPTO_MODULE_DIR}/drivers/builtin/include/mbedtls/private")
  set(legacy_headers
    ${MBEDTLS_PRIVATE_INCLUDE_PATH}/bignum.h
    ${MBEDTLS_PRIVATE_INCLUDE_PATH}/cipher.h
    ${MBEDTLS_PRIVATE_INCLUDE_PATH}/cmac.h
    ${MBEDTLS_PRIVATE_INCLUDE_PATH}/ecdsa.h
    ${MBEDTLS_PRIVATE_INCLUDE_PATH}/ecp.h
    ${MBEDTLS_PRIVATE_INCLUDE_PATH}/pkcs5.h
    ${MBEDTLS_PRIVATE_INCLUDE_PATH}/error_common.h
    ${MBEDTLS_PRIVATE_INCLUDE_PATH}/sha256.h
    ${MBEDTLS_PRIVATE_INCLUDE_PATH}/rsa.h
  )
  file(COPY ${legacy_headers} DESTINATION ${CMAKE_BINARY_DIR}/legacy-mbedtls-headers/mbedtls/)
  target_include_directories(mbedTLS INTERFACE
    ${CMAKE_BINARY_DIR}/legacy-mbedtls-headers/
  )
endif()
