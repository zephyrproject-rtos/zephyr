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

  zephyr_library_sources(library/aes.c)
  zephyr_library_sources(library/aesni.c)
  zephyr_library_sources(library/arc4.c)
  zephyr_library_sources(library/aria.c)
  zephyr_library_sources(library/asn1parse.c)
  zephyr_library_sources(library/asn1write.c)
  zephyr_library_sources(library/base64.c)
  zephyr_library_sources(library/bignum.c)
  zephyr_library_sources(library/blowfish.c)
  zephyr_library_sources(library/camellia.c)
  zephyr_library_sources(library/ccm.c)
  zephyr_library_sources(library/certs.c)
  zephyr_library_sources(library/chacha20.c)
  zephyr_library_sources(library/chachapoly.c)
  zephyr_library_sources(library/cipher.c)
  zephyr_library_sources(library/cipher_wrap.c)
  zephyr_library_sources(library/cmac.c)
  zephyr_library_sources(library/ctr_drbg.c)
  zephyr_library_sources(library/debug.c)
  zephyr_library_sources(library/des.c)
  zephyr_library_sources(library/dhm.c)
  zephyr_library_sources(library/ecdh.c)
  zephyr_library_sources(library/ecdsa.c)
  zephyr_library_sources(library/ecjpake.c)
  zephyr_library_sources(library/ecp.c)
  zephyr_library_sources(library/ecp_curves.c)
  zephyr_library_sources(library/entropy.c)
  zephyr_library_sources(library/entropy_poll.c)
  zephyr_library_sources(library/error.c)
  zephyr_library_sources(library/gcm.c)
  zephyr_library_sources(library/havege.c)
  zephyr_library_sources(library/hkdf.c)
  zephyr_library_sources(library/hmac_drbg.c)
  zephyr_library_sources(library/md.c)
  zephyr_library_sources(library/md2.c)
  zephyr_library_sources(library/md4.c)
  zephyr_library_sources(library/md5.c)
  zephyr_library_sources(library/md_wrap.c)
  zephyr_library_sources(library/memory_buffer_alloc.c)
  zephyr_library_sources(library/net_sockets.c)
  zephyr_library_sources(library/nist_kw.c)
  zephyr_library_sources(library/oid.c)
  zephyr_library_sources(library/padlock.c)
  zephyr_library_sources(library/pem.c)
  zephyr_library_sources(library/pk.c)
  zephyr_library_sources(library/pk_wrap.c)
  zephyr_library_sources(library/pkcs11.c)
  zephyr_library_sources(library/pkcs12.c)
  zephyr_library_sources(library/pkcs5.c)
  zephyr_library_sources(library/pkparse.c)
  zephyr_library_sources(library/pkwrite.c)
  zephyr_library_sources(library/platform.c)
  zephyr_library_sources(library/platform_util.c)
  zephyr_library_sources(library/poly1305.c)
  zephyr_library_sources(library/ripemd160.c)
  zephyr_library_sources(library/rsa.c)
  zephyr_library_sources(library/rsa_internal.c)
  zephyr_library_sources(library/sha1.c)
  zephyr_library_sources(library/sha256.c)
  zephyr_library_sources(library/sha512.c)
  zephyr_library_sources(library/ssl_cache.c)
  zephyr_library_sources(library/ssl_ciphersuites.c)
  zephyr_library_sources(library/ssl_cli.c)
  zephyr_library_sources(library/ssl_cookie.c)
  zephyr_library_sources(library/ssl_srv.c)
  zephyr_library_sources(library/ssl_ticket.c)
  zephyr_library_sources(library/ssl_tls.c)
  zephyr_library_sources(library/threading.c)
  zephyr_library_sources(library/timing.c)
  zephyr_library_sources(library/version.c)
  zephyr_library_sources(library/version_features.c)
  zephyr_library_sources(library/x509.c)
  zephyr_library_sources(library/x509_create.c)
  zephyr_library_sources(library/x509_crl.c)
  zephyr_library_sources(library/x509_crt.c)
  zephyr_library_sources(library/x509_csr.c)
  zephyr_library_sources(library/x509write_crt.c)
  zephyr_library_sources(library/x509write_csr.c)
  zephyr_library_sources(library/xtea.c)

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
