if(CONFIG_ZCBOR)
  zephyr_include_directories(
    ${ZEPHYR_ZCBOR_MODULE_DIR}/include
  )

  zephyr_library()
  zephyr_library_sources(
    ${ZEPHYR_ZCBOR_MODULE_DIR}/src/zcbor_common.c
    ${ZEPHYR_ZCBOR_MODULE_DIR}/src/zcbor_decode.c
    ${ZEPHYR_ZCBOR_MODULE_DIR}/src/zcbor_encode.c
  )

  zephyr_compile_definitions_ifdef(CONFIG_ZCBOR_CANONICAL ZCBOR_CANONICAL)
  zephyr_compile_definitions_ifdef(CONFIG_ZCBOR_STOP_ON_ERROR ZCBOR_STOP_ON_ERROR)
  zephyr_compile_definitions_ifdef(CONFIG_ZCBOR_VERBOSE ZCBOR_VERBOSE)
  zephyr_compile_definitions_ifdef(CONFIG_ZCBOR_ASSERT ZCBOR_ASSERTS)
  zephyr_compile_definitions_ifdef(CONFIG_ZCBOR_BIG_ENDIAN ZCBOR_BIG_ENDIAN)
endif()
