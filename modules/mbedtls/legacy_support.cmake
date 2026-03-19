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

set(MBEDTLS_EXPORT_REMOVED_HEADERS  OFF)
set(MBEDTLS_REMOVED_MODULES_PATH "${ZEPHYR_HOSTAP_MODULE_DIR}/port/mbedtls/removed")

# HostAP still heavily depends on legacy Mbed TLS crypto. Luckily most of
# that support is still available and it can still be accessed somehow,
# but DHM and DES modules were removed from tf-psa-crypto. As a temporary solution
# they are added back in TF-PSA-Crypto and they are compiled with the
# builtin library when crypto features of HostAP library are enabled.
if(CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ALT)
  target_sources(builtin PRIVATE ${MBEDTLS_REMOVED_MODULES_PATH}/dhm.c)
  target_sources(builtin PRIVATE ${MBEDTLS_REMOVED_MODULES_PATH}/des.c)
  # These build symbols cannot be set in tf-psa-crypto configuration header
  # file otherwise the build will fail, so we manually set them here only
  # to build these legacy modules.
  target_compile_definitions(builtin PRIVATE
    # Setting legacy build symbols is not allowed so we need to set this
    # to bypass the check.
    -DTF_PSA_CRYPTO_CONFIG_CHECK_BYPASS
    -DMBEDTLS_DES_C
    -DMBEDTLS_DHM_C
  )
  set(MBEDTLS_EXPORT_REMOVED_HEADERS  ON)
endif()

# This is required because ESP32 drivers for BT and WiFi still rely on legacy
# crypto.
if(CONFIG_ESP32_BT_LE_CRYPTO_STACK_MBEDTLS OR CONFIG_ESP32_WIFI_MBEDTLS_CRYPTO)
  target_sources(builtin PRIVATE ${MBEDTLS_REMOVED_MODULES_PATH}/ecdh.c)
  target_compile_definitions(builtin PRIVATE
    # Setting legacy build symbols is not allowed so we need to set this
    # to bypass the check.
    -DTF_PSA_CRYPTO_CONFIG_CHECK_BYPASS
    # MBEDTLS_ECP_C, MBEDTLS_BIGNUM_C, MBEDTLS_ECP_DP_SECP256R1_ENABLED
    # are auto-derived from PSA_WANT_* by crypto_adjust_config_enable_builtins.h.
    # MBEDTLS_ENTROPY_C was removed in TF-PSA-Crypto 1.0 (now implied).
    # Only define symbols not auto-derived:
    -DMBEDTLS_ECDH_C
  )
  set(MBEDTLS_EXPORT_REMOVED_HEADERS  ON)
endif()

if(MBEDTLS_EXPORT_REMOVED_HEADERS)
  target_include_directories(builtin PRIVATE ${MBEDTLS_REMOVED_MODULES_PATH})
  target_include_directories(mbedTLS INTERFACE
    ${MBEDTLS_REMOVED_MODULES_PATH}
  )
endif()
