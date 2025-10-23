if(CONFIG_LIBSBC_ENCODER OR CONFIG_LIBSBC_DECODER)

zephyr_library_named(libsbc)
zephyr_library_compile_options(-O3 -std=c11 -ffast-math -Wno-array-bounds)

zephyr_compile_definitions(SBC_FOR_EMBEDDED_LINUX)
zephyr_compile_definitions(SBC_NO_PCM_CPY_OPTION)
zephyr_library_sources(sbc.c)

zephyr_include_directories(
    ${ZEPHYR_LIBSBC_MODULE_DIR}/encoder/include
  )

zephyr_include_directories(
  ${ZEPHYR_LIBSBC_MODULE_DIR}/decoder/include
  ${ZEPHYR_LIBSBC_MODULE_DIR}/decoder/srce
  )

if(CONFIG_LIBSBC_ENCODER)
zephyr_library_sources(
  ${ZEPHYR_LIBSBC_MODULE_DIR}/encoder/srce/sbc_analysis.c
  ${ZEPHYR_LIBSBC_MODULE_DIR}/encoder/srce/sbc_dct.c
  ${ZEPHYR_LIBSBC_MODULE_DIR}/encoder/srce/sbc_dct_coeffs.c
  ${ZEPHYR_LIBSBC_MODULE_DIR}/encoder/srce/sbc_enc_bit_alloc_mono.c
  ${ZEPHYR_LIBSBC_MODULE_DIR}/encoder/srce/sbc_enc_bit_alloc_ste.c
  ${ZEPHYR_LIBSBC_MODULE_DIR}/encoder/srce/sbc_enc_coeffs.c
  ${ZEPHYR_LIBSBC_MODULE_DIR}/encoder/srce/sbc_encoder.c
  ${ZEPHYR_LIBSBC_MODULE_DIR}/encoder/srce/sbc_packing.c
  )
endif()

if(CONFIG_LIBSBC_DECODER)
zephyr_library_sources(
  ${ZEPHYR_LIBSBC_MODULE_DIR}/decoder/srce/alloc.c
  ${ZEPHYR_LIBSBC_MODULE_DIR}/decoder/srce/bitalloc.c
  ${ZEPHYR_LIBSBC_MODULE_DIR}/decoder/srce/bitalloc-sbc.c
  ${ZEPHYR_LIBSBC_MODULE_DIR}/decoder/srce/bitstream-decode.c
  ${ZEPHYR_LIBSBC_MODULE_DIR}/decoder/srce/decoder-oina.c
  ${ZEPHYR_LIBSBC_MODULE_DIR}/decoder/srce/decoder-private.c
  ${ZEPHYR_LIBSBC_MODULE_DIR}/decoder/srce/decoder-sbc.c
  ${ZEPHYR_LIBSBC_MODULE_DIR}/decoder/srce/dequant.c
  ${ZEPHYR_LIBSBC_MODULE_DIR}/decoder/srce/framing.c
  ${ZEPHYR_LIBSBC_MODULE_DIR}/decoder/srce/framing-sbc.c
  ${ZEPHYR_LIBSBC_MODULE_DIR}/decoder/srce/oi_codec_version.c
  ${ZEPHYR_LIBSBC_MODULE_DIR}/decoder/srce/readsamplesjoint.inc
  ${ZEPHYR_LIBSBC_MODULE_DIR}/decoder/srce/synthesis-8-generated.c
  ${ZEPHYR_LIBSBC_MODULE_DIR}/decoder/srce/synthesis-dct8.c
  ${ZEPHYR_LIBSBC_MODULE_DIR}/decoder/srce/synthesis-sbc.c
  )
endif()
endif()
