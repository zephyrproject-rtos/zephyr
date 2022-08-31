if(CONFIG_MBEDTLS)
zephyr_interface_library_named(mbedTLS)

if(CONFIG_MBEDTLS_BUILTIN)
  target_compile_definitions(mbedTLS INTERFACE
    MBEDTLS_CONFIG_FILE="${CONFIG_MBEDTLS_CFG_FILE}"
  )

  target_include_directories(mbedTLS INTERFACE
    ${ZEPHYR_CURRENT_MODULE_DIR}/include
    # mbedTLS v3.1.0 has psa_crypto_cipher.c including an internal header using <>.
    # This line ensures the header can be found in Zephyr.
    # When updating to mbedTLS > v3.1.0, please check if this has been corrected,
    # and if so remove this include path.
    ${ZEPHYR_CURRENT_MODULE_DIR}/library
    configs
    include
  )

  zephyr_library()

  file(GLOB
    mbedtls_sources # This is an output parameter
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/*.c
    )

  if(CONFIG_MBEDTLS_DEBUG_EXTRACT_BASENAME_AT_BUILDTIME)
    zephyr_cc_option(-fmacro-prefix-map=${ZEPHYR_CURRENT_MODULE_DIR}/library/=)
  endif()

  zephyr_library_sources(
    zephyr_init.c
    ${mbedtls_sources}
  )

  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_DEBUG debug.c)
  zephyr_library_sources_ifdef(CONFIG_MBEDTLS_SHELL shell.c)

  # mbedTLS v3.1.0 is having unused variables and functions in /library/ssl_msg.c
  # To avoid compilation warnings, which are treated as errors in CI, we disable unused variables and functions.
  # Please check when mbedTLS is updated to version >v3.1.0 if those flags are still needed.
  # If mbedTLS has fixed the mentioned issue, then please remove the flags.
  set_source_files_properties(${ZEPHYR_CURRENT_MODULE_DIR}/library/ssl_msg.c
      PROPERTIES COMPILE_OPTIONS "-Wno-unused-variable;-Wno-unused-function")

  zephyr_library_app_memory(k_mbedtls_partition)
  if(CONFIG_ARCH_POSIX AND CONFIG_ASAN AND NOT CONFIG_64BIT)
    # i386 assembly code used in MBEDTLS does not compile with size optimization
    # if address sanitizer is enabled, as such switch default optimization level
    # to speed
    set_property(SOURCE ${ZEPHYR_CURRENT_MODULE_DIR}/mbedtls/library/bignum.c APPEND PROPERTY COMPILE_OPTIONS
        "${OPTIMIZE_FOR_SPEED_FLAG}")
  endif ()

  if(CONFIG_MBEDTLS_ZEPHYR_ENTROPY AND NOT CONFIG_ENTROPY_HAS_DRIVER)
    message(WARNING "No entropy device on the system, using fake entropy source!")
  endif()

  zephyr_library_link_libraries(mbedTLS)
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
