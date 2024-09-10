# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(jwt.c)

zephyr_library_sources_ifdef(CONFIG_JWT_SIGN_ECDSA_LEGACY jwt_legacy_ecdsa.c)
zephyr_library_sources_ifdef(CONFIG_JWT_SIGN_RSA_LEGACY jwt_legacy_rsa.c)
zephyr_library_sources_ifdef(CONFIG_JWT_USE_PSA jwt_psa.c)

zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)
