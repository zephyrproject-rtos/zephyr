# Copyright (c) 2026 BayLibre SAS
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_MBEDTLS_DECLARE_PRIVATE_IDENTIFIERS)
  message(WARNING "
    Enabling CONFIG_MBEDTLS_DECLARE_PRIVATE_IDENTIFIERS is discouraged as it
    gives access to Mbed TLS crypto functions which are internal and may be removed
    or modified at any time. Please transition to the PSA Crypto API."
  )
  if(CONFIG_MCUBOOT)
    # MCUBoot bootutil includes rsa_alt_helpers.h by basename; the header lives
    # next to builtin RSA sources under drivers/builtin/src.
    target_include_directories(mbedtls_iface INTERFACE
      ${ZEPHYR_TF_PSA_CRYPTO_MODULE_DIR}/drivers/builtin/src
    )
  endif()
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
  # In PSA client mode (e.g. TF-M NS builds) MBEDTLS_PSA_CRYPTO_C is not
  # set, so crypto_adjust_config_enable_builtins.h is skipped in the config
  # processing chain (build_info.h guards it with MBEDTLS_PSA_CRYPTO_C).
  # Without it MBEDTLS_ECP_C, MBEDTLS_BIGNUM_C and the curve macros are
  # never derived from PSA_WANT_*.  The builtin source files (ecp.c,
  # bignum.c, ecp_curves.c) are always GLOBbed into the builtin target
  # but compile to empty without these symbols.  Manually define what
  # enable_builtins.h would have set so the implementations are compiled
  # (PRIVATE on builtin) and the declarations are visible to consumers
  # (INTERFACE on Mbed TLS).
  #
  # These symbols are flagged as removed by the tf-psa-crypto config
  # validation in tf_psa_crypto_config.c.  The builtin target already
  # has TF_PSA_CRYPTO_CONFIG_CHECK_BYPASS; extend the bypass to the
  # tfpsacrypto core target so the INTERFACE definitions do not trip
  # the check there either.
  if(CONFIG_BUILD_WITH_TFM)
    target_compile_definitions(builtin PRIVATE
      MBEDTLS_ECP_C
      MBEDTLS_BIGNUM_C
      MBEDTLS_ECP_DP_SECP256R1_ENABLED
      MBEDTLS_ECP_DP_SECP384R1_ENABLED
      MBEDTLS_ECP_DP_SECP521R1_ENABLED
      MBEDTLS_ECP_DP_BP256R1_ENABLED
      MBEDTLS_ECP_DP_BP384R1_ENABLED
      MBEDTLS_ECP_DP_BP512R1_ENABLED
      MBEDTLS_ECP_DP_CURVE25519_ENABLED
      MBEDTLS_ECP_DP_CURVE448_ENABLED
    )
    target_compile_definitions(mbedtls_iface INTERFACE
      MBEDTLS_ECP_C
      MBEDTLS_BIGNUM_C
      MBEDTLS_ECP_DP_SECP256R1_ENABLED
      MBEDTLS_ECP_DP_SECP384R1_ENABLED
      MBEDTLS_ECP_DP_SECP521R1_ENABLED
      MBEDTLS_ECP_DP_BP256R1_ENABLED
      MBEDTLS_ECP_DP_BP384R1_ENABLED
      MBEDTLS_ECP_DP_BP512R1_ENABLED
      MBEDTLS_ECP_DP_CURVE25519_ENABLED
      MBEDTLS_ECP_DP_CURVE448_ENABLED
    )
    target_compile_definitions(tfpsacrypto PRIVATE
      TF_PSA_CRYPTO_CONFIG_CHECK_BYPASS
    )
  endif()
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
  target_include_directories(mbedtls_iface INTERFACE
    ${MBEDTLS_REMOVED_MODULES_PATH}
  )
endif()
