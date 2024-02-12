#
# Copyright (c) 2023 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0
#
if(CONFIG_MBEDTLS)
zephyr_interface_library_named(mbedTLS)

  if(CONFIG_MBEDTLS_BUILTIN)
    if(CONFIG_MBEDTLS_ENTROPY_POLL_ZEPHYR AND NOT CONFIG_ENTROPY_HAS_DRIVER)
      message(WARNING "No entropy device on the system, using fake entropy source!")
    endif()

    if(CONFIG_MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG)
      if(CONFIG_MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG_ALLOW_NON_CSPRNG OR
         CONFIG_TEST_CSPRNG_GENERATOR)
        message(WARNING "
          Non cryptographycally secure sources are enabled for psa_generate_random().
          This is meant to be used only for tests, not in production!")
      else()
        if(NOT CONFIG_CSPRNG_ENABLED)
          message(FATAL_ERROR "
            MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG is set but there is
            no CSPRNG enabled.")
        endif()
      endif()
    endif()

    # Add the config-file entry point
    target_compile_definitions(mbedTLS INTERFACE
        MBEDTLS_CONFIG_FILE="${CONFIG_MBEDTLS_CFG_FILE}"
    )

    if (CONFIG_BUILD_WITH_TFM)
      target_include_directories(mbedTLS INTERFACE
        $<TARGET_PROPERTY:tfm,TFM_BINARY_DIR>/api_ns/interface/include
      )
    endif()

    # Add regular includes
    target_include_directories(mbedTLS INTERFACE
        ${ZEPHYR_CURRENT_MODULE_DIR}/include
        ${ZEPHYR_CURRENT_MODULE_DIR}/include/library
        ${ZEPHYR_CURRENT_MODULE_DIR}/library
        configs
        include
    )

    if (CONFIG_MBEDTLS_PSA_P256M_DRIVER_RAW)
      target_include_directories(mbedTLS INTERFACE
        ${ZEPHYR_CURRENT_MODULE_DIR}/3rdparty/p256-m/p256-m
      )
    endif()

    # Add base library with files required by all drivers/backends.
    zephyr_library_named(mbedTLSBase)

    # Base mbed TLS files
    list(APPEND mbedtls_base_src
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/aes.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/aesni.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/aria.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/asn1parse.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/asn1write.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/base64.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/bignum_core.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/bignum_mod_raw.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/bignum_mod.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/bignum.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/block_cipher.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/camellia.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/ccm.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/chacha20.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/chachapoly.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/cipher_wrap.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/cipher.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/cmac.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/constant_time.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/ctr_drbg.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/debug.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/des.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/dhm.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/ecdh.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/ecdsa.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/ecjpake.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/ecp_curves_new.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/ecp_curves.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/ecp.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/entropy_poll.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/entropy.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/error.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/gcm.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/hkdf.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/hmac_drbg.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/lmots.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/lms.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/md.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/md5.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/memory_buffer_alloc.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/mps_reader.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/mps_trace.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/nist_kw.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/oid.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/padlock.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/platform_util.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/platform.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/poly1305.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/psa_util.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/ripemd160.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/rsa_alt_helpers.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/rsa.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/sha1.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/sha256.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/sha512.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/sha3.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/threading.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/timing.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/version_features.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/version.c
      zephyr_init.c
      zephyr_entropy.c
    )

    zephyr_library_sources(${mbedtls_base_src})

    zephyr_library_sources_ifdef(CONFIG_MBEDTLS_DEBUG debug.c)
    zephyr_library_sources_ifdef(CONFIG_MBEDTLS_SHELL shell.c)

    zephyr_library_app_memory(k_mbedtls_partition)
    if(CONFIG_ARCH_POSIX AND CONFIG_ASAN AND NOT CONFIG_64BIT)
      # i386 assembly code used in MBEDTLS does not compile with size optimization
      # if address sanitizer is enabled, as such switch default optimization level
      # to speed
      set_property(SOURCE ${ZEPHYR_CURRENT_MODULE_DIR}/mbedtls/library/bignum.c APPEND PROPERTY COMPILE_OPTIONS
          "${OPTIMIZE_FOR_SPEED_FLAG}")
    endif ()

    zephyr_library_link_libraries(mbedTLS)

    zephyr_library_named(mbedTLSCrypto)

    if (CONFIG_MBEDTLS_PSA_CRYPTO_C)
      list(APPEND crypto_source
        ${ZEPHYR_CURRENT_MODULE_DIR}/library/psa_crypto_aead.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/library/psa_crypto_cipher.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/library/psa_crypto_driver_wrappers_no_static.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/library/psa_crypto_ecp.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/library/psa_crypto_ffdh.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/library/psa_crypto_hash.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/library/psa_crypto_mac.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/library/psa_crypto_pake.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/library/psa_crypto_rsa.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/library/psa_crypto_se.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/library/psa_crypto_storage.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/library/psa_its_file.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/library/psa_crypto.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/library/psa_crypto_client.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/library/psa_crypto_slot_management.c
      )
    endif()

    if(CONFIG_MBEDTLS_PSA_P256M_DRIVER_ENABLED)
      list(APPEND crypto_source
        ${ZEPHYR_CURRENT_MODULE_DIR}/3rdparty/p256-m/p256-m_driver_entrypoints.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/3rdparty/p256-m/p256-m/p256-m.c
      )
      zephyr_library_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/library)
    endif()

    list(APPEND crypto_source
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/pem.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/pkcs12.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/pkcs5.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/pkparse.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/pkwrite.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/pk.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/pk_ecc.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/pk_wrap.c
    )

    zephyr_library_sources(${crypto_source})

    # Custom macro to tell that an mbedTLSCrypto source file is being compiled.
    zephyr_library_compile_definitions(BUILDING_MBEDTLS_CRYPTO)

    zephyr_library_link_libraries(mbedTLS)

    zephyr_library_link_libraries_ifdef(CONFIG_BUILD_WITH_TFM tfm_api)

    zephyr_library_named(mbedTLSX509)

    list(APPEND x509_source
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/x509.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/x509_create.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/x509_crl.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/x509_crt.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/x509_csr.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/x509write_crt.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/x509write_csr.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/x509write.c
    )

    zephyr_library_sources(${x509_source})

    zephyr_library_link_libraries(mbedTLS)

    zephyr_library()

    list(APPEND mbedtls_source
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/net_sockets.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/ssl_cache.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/ssl_ciphersuites.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/ssl_client.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/ssl_cookie.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/ssl_debug_helpers_generated.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/ssl_msg.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/ssl_ticket.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/ssl_tls12_client.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/ssl_tls12_server.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/ssl_tls13_client.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/ssl_tls13_generic.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/ssl_tls13_keys.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/ssl_tls13_server.c
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/ssl_tls.c
    )

    zephyr_library_sources(${mbedtls_source})

    zephyr_library_link_libraries(
      mbedTLSX509
      mbedTLSCrypto
      mbedTLSBase
      mbedTLS
    )

elseif (CONFIG_MBEDTLS_LIBRARY)

  # NB: CONFIG_MBEDTLS_LIBRARY is not regression tested and is
  # therefore susceptible to bit rot
  target_include_directories(mbedTLS INTERFACE
    ${CONFIG_MBEDTLS_INSTALL_PATH}
  )
  zephyr_link_libraries(
    mbedtls_external
    -L${CONFIG_MBEDTLS_INSTALL_PATH}
    gcc
    )
  # Lib mbedtls_external depends on libgcc (I assume?) so to allow
  # mbedtls_external to link with gcc we need to ensure it is placed
  # after mbedtls_external on the linkers command line.
else()
  # If none of either CONFIG_MBEDTLS_BUILTIN or CONFIG_MBEDTLS_LIBRARY
  # are defined the users need add a custom Kconfig choice to the
  # MBEDTLS_IMPLEMENTATION and manually add the mbedtls library and
  # included the required directories for mbedtls in their projects.
endif()

endif()
