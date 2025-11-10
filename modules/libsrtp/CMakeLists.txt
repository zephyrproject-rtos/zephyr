# Copyright (c) 2025 Sayed Naser Moravej <seyednasermoravej@gmail.com>
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_LIBSRTP)

zephyr_include_directories(
  ${ZEPHYR_LIBSRTP_MODULE_DIR}/include
  ${ZEPHYR_LIBSRTP_MODULE_DIR}/crypto/include
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${ZEPHYR_LIBSRTP_MODULE_DIR}/test
)

zephyr_library_named(libsrtp)

zephyr_library_compile_definitions(MBEDTLS)

zephyr_library_sources(
  ${ZEPHYR_LIBSRTP_MODULE_DIR}/srtp/srtp.c
  ${ZEPHYR_LIBSRTP_MODULE_DIR}/crypto/cipher/cipher.c
  ${ZEPHYR_LIBSRTP_MODULE_DIR}/crypto/cipher/cipher_test_cases.c
  ${ZEPHYR_LIBSRTP_MODULE_DIR}/crypto/cipher/null_cipher.c
  ${ZEPHYR_LIBSRTP_MODULE_DIR}/crypto/math/datatypes.c
  ${ZEPHYR_LIBSRTP_MODULE_DIR}/crypto/kernel/alloc.c
  ${ZEPHYR_LIBSRTP_MODULE_DIR}/crypto/kernel/key.c
  ${ZEPHYR_LIBSRTP_MODULE_DIR}/crypto/kernel/err.c
  ${ZEPHYR_LIBSRTP_MODULE_DIR}/crypto/kernel/crypto_kernel.c
  ${ZEPHYR_LIBSRTP_MODULE_DIR}/crypto/replay/rdb.c
  ${ZEPHYR_LIBSRTP_MODULE_DIR}/crypto/replay/rdbx.c
  ${ZEPHYR_LIBSRTP_MODULE_DIR}/crypto/hash/auth.c
  ${ZEPHYR_LIBSRTP_MODULE_DIR}/crypto/hash/auth_test_cases.c
  ${ZEPHYR_LIBSRTP_MODULE_DIR}/test/rtp.c
  ${ZEPHYR_LIBSRTP_MODULE_DIR}/crypto/hash/null_auth.c
  ${ZEPHYR_LIBSRTP_MODULE_DIR}/test/util.c
  ${ZEPHYR_LIBSRTP_MODULE_DIR}/crypto/hash/hmac_mbedtls.c
  ${ZEPHYR_LIBSRTP_MODULE_DIR}/crypto/cipher/aes_icm_mbedtls.c
  ${ZEPHYR_LIBSRTP_MODULE_DIR}/crypto/cipher/aes_gcm_mbedtls.c
)

zephyr_library_compile_definitions(libsrtp HAVE_CONFIG_H)

zephyr_library_link_libraries(mbedTLS)

endif()
