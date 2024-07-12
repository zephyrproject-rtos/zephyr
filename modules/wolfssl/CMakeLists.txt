if(CONFIG_WOLFSSL)
  zephyr_interface_library_named(wolfSSL)

  if(CONFIG_WOLFSSL_BUILTIN)
    if(CONFIG_WOLFSSL_SETTINGS_FILE)
      target_compile_definitions(wolfSSL INTERFACE
        WOLFSSL_SETTINGS_FILE="${CONFIG_WOLFSSL_SETTINGS_FILE}"
      )
      zephyr_include_directories(
        ${APPLICATION_CONFIG_DIR}
        ${APPLICATION_CONFIG_DIR}/src
      )
    endif()

    zephyr_include_directories(
  	${ZEPHYR_CURRENT_MODULE_DIR}
  	${ZEPHYR_CURRENT_MODULE_DIR}/wolfssl
  	${ZEPHYR_CURRENT_MODULE_DIR}/zephyr
  	)

    zephyr_library()
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/zephyr/zephyr_init.c)

    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/src/crl.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/src/dtls.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/src/dtls13.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/src/internal.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/src/keys.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/src/ocsp.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/src/sniffer.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/src/ssl.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/src/tls.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/src/tls13.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/src/wolfio.c)

    # FIPS Boundary
    if(CONFIG_WOLFCRYPT_FIPS)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/wolfcrypt_first.c)
    endif()
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/hmac.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/random.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/kdf.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/rsa.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/ecc.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/aes.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/sha.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/sha256.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/sha512.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/sha3.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/dh.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/cmac.c)
    if(CONFIG_WOLFCRYPT_FIPS)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/fips.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/fips_test.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/wolfcrypt_last.c)
    endif()

    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/arc4.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/asm.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/asn.c)
    #zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/async.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/blake2b.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/blake2s.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/camellia.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/chacha.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/chacha20_poly1305.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/coding.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/compress.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/cpuid.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/cryptocb.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/curve25519.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/curve448.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/des3.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/dilithium.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/dsa.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/ecc_fp.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/eccsi.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/ed25519.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/ed448.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/error.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/ext_kyber.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/falcon.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/fe_448.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/fe_low_mem.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/fe_operations.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/ge_448.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/ge_low_mem.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/ge_operations.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/hash.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/integer.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/logging.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/md2.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/md4.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/md5.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/memory.c)
    #zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/misc.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/pkcs12.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/pkcs7.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/poly1305.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/pwdbased.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/ripemd.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/sakke.c)
    #zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/selftest.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/signature.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/siphash.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/sp_arm32.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/sp_arm64.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/sp_armthumb.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/sp_c32.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/sp_c64.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/sp_cortexm.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/sp_dsp32.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/sp_int.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/sp_x86_64.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/sphincs.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/srp.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/tfm.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/wc_dsp.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/wc_encrypt.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/wc_pkcs11.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/wc_port.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/wolfevent.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/wolfmath.c)

    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/port/liboqs/liboqs.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/port/psa/psa.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/port/psa/psa_aes.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/port/psa/psa_hash.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/port/psa/psa_pkcbs.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/port/st/stm32.c)

    if(CONFIG_WOLFCRYPT_ARMASM)
      # tested with board: "qemu_kvm_arm64"
      zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/port/arm/armv8-aes.c)
      zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/port/arm/armv8-32-aes-asm_c.c)
      zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/port/arm/armv8-sha256.c)
      zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/port/arm/armv8-32-sha256-asm_c.c)
      zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/port/arm/armv8-sha512.c)
      zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/port/arm/armv8-sha512-asm_c.c)
      zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/port/arm/armv8-sha3-asm_c.c)
      zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/port/arm/armv8-poly1305.c)
      zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/port/arm/armv8-chacha.c)
      zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/port/arm/armv8-curve25519_c.c)

      # Note: The cmake/gcc-m-cpu.cmake make need updated to add "+crypto -mstrict-align"
      set(TOOLCHAIN_C_FLAGS "-mcpu=cortex-a53+crypto -mstrict-align")
    endif()

    if(CONFIG_WOLFCRYPT_INTELASM)
      # tested with board: "qemu_x86_64"
      zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/sha256_asm.S)
      zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/sha512_asm.S)
      zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/sha3_asm.S)
      zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/chacha_asm.S)
      zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/poly1305_asm.S)

      # AESNI
      zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/aes_asm.S)
      zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/wolfcrypt/src/aes_gcm_x86_asm.S)
      set(TOOLCHAIN_C_FLAGS "-march=native -maes -msse4 -mpclmul ")
    endif()

    zephyr_library_link_libraries(wolfSSL)

  	target_compile_definitions(wolfSSL INTERFACE WOLFSSL_ZEPHYR)
  	target_compile_definitions(wolfSSL INTERFACE WOLFSSL_USER_SETTINGS)
    if(CONFIG_WOLFSSL_DEBUG)
      target_compile_definitions(wolfSSL INTERFACE DEBUG_WOLFSSL)
      zephyr_library_compile_options(-g3 -O0)
    endif()
  else()
    assert(CONFIG_WOLFSSL_LIBRARY "wolfSSL was enabled, but neither BUILTIN or LIBRARY was selected.")

    # NB: CONFIG_WOLFSSL_LIBRARY is not regression tested and is
    # therefore susceptible to bit rot

    target_include_directories(wolfSSL INTERFACE
  	${CONFIG_WOLFSSL_INSTALL_PATH}
  	)

    zephyr_link_libraries(
      wolfssl_external
      -L${CONFIG_WOLFSSL_INSTALL_PATH}
      gcc
      )
    # Lib wolfssl depends on libgcc so to allow
    # wolfssl to link with gcc we need to ensure it is placed
    # after wolfssl_external on the linkers command line.
  endif()

  target_link_libraries(wolfSSL INTERFACE zephyr_interface)

endif()
