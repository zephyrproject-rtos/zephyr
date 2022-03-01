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
endif()
