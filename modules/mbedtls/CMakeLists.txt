if(CONFIG_MBEDTLS)
zephyr_interface_library_named(mbedTLS)

if(CONFIG_MBEDTLS_BUILTIN)
  target_compile_definitions(mbedTLS INTERFACE
	  MBEDTLS_CONFIG_FILE="${CONFIG_MBEDTLS_CFG_FILE}"
	)

  target_include_directories(mbedTLS INTERFACE
    ${ZEPHYR_CURRENT_MODULE_DIR}/include
    configs
	)

  zephyr_library()

  file(GLOB
    mbedtls_sources # This is an output parameter
      ${ZEPHYR_CURRENT_MODULE_DIR}/library/*.c
    )

  zephyr_library_sources(
    zephyr_init.c
    ${mbedtls_sources}
  )

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
