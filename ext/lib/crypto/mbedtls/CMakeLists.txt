zephyr_interface_library_named(mbedTLS)

if(CONFIG_MBEDTLS_BUILTIN)
  target_compile_definitions(mbedTLS INTERFACE
	MBEDTLS_CONFIG_FILE="${CONFIG_MBEDTLS_CFG_FILE}"
	)

  target_include_directories(mbedTLS INTERFACE
	include
	configs
	)

  zephyr_library()
  zephyr_library_sources(zephyr_init.c)

  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/aes.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/aesni.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/arc4.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/asn1parse.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/asn1write.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_BASE64 library/base64.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/bignum.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/blowfish.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/camellia.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/ccm.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/certs.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/cipher.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/cipher_wrap.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/cmac.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/ctr_drbg.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/debug.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/des.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/dhm.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/ecdh.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/ecdsa.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/ecjpake.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/ecp.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/ecp_curves.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/entropy.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/entropy_poll.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/error.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/gcm.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/havege.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/hmac_drbg.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/md.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/md2.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/md4.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/md5.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/md_wrap.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/memory_buffer_alloc.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/net_sockets.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/oid.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/padlock.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/pem.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/pk.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/pk_wrap.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/pkcs11.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/pkcs12.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/pkcs5.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/pkparse.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/pkwrite.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/platform.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/ripemd160.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/rsa.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/rsa_internal.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/sha1.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/sha256.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/sha512.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/ssl_cache.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/ssl_ciphersuites.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/ssl_cli.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/ssl_cookie.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/ssl_srv.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/ssl_ticket.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/ssl_tls.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/threading.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/timing.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/version.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/version_features.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/x509.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/x509_create.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/x509_crl.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/x509_crt.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/x509_csr.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/x509write_crt.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/x509write_csr.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_CRYPTO library/xtea.c)

  zephyr_library_link_libraries(mbedTLS)
else()
  assert(CONFIG_MBEDTLS_LIBRARY "MBEDTLS was enabled, but neither BUILTIN or LIBRARY was selected.")

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
endif()

target_link_libraries(mbedTLS INTERFACE zephyr_interface)
