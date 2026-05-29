if(CONFIG_LIBSBC)

zephyr_library_named(libsbc)
zephyr_library_compile_options(-O3 -std=c11 -ffast-math)
# -Wno-stringop-overflow was added as otherwise gcc 13.3 produces multiple warnings in this library
# Newer version of gcc do not produce them, and this code is taken from Android where it was
# validated. So these warnings are presumed to be false positives.
if(CMAKE_C_COMPILER_ID MATCHES "GNU")
  zephyr_library_compile_options(-Wno-stringop-overflow)
endif()

zephyr_include_directories(
    ${ZEPHYR_LIBSBC_MODULE_DIR}/encoder/include
  )

zephyr_include_directories(
  ${ZEPHYR_LIBSBC_MODULE_DIR}/decoder/include
  ${ZEPHYR_LIBSBC_MODULE_DIR}/decoder/srce
  )

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
