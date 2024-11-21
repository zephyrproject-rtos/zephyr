# Copyright (c) 2024 Telink Semiconductor
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_LZMA)

  set(LZMA_DIR ${ZEPHYR_CURRENT_MODULE_DIR})

  add_library(liblzma STATIC
    ${LZMA_DIR}/src/liblzma/common/common.c
    ${LZMA_DIR}/src/liblzma/lzma/lzma_decoder.c
    ${LZMA_DIR}/src/liblzma/lz/lz_decoder.c
    ${LZMA_DIR}/src/liblzma/common/filter_decoder.c
    ${LZMA_DIR}/src/liblzma/common/filter_common.c
  )

  zephyr_library_include_directories(
    ${LZMA_DIR}/src/
  )

  target_include_directories(liblzma PRIVATE
    configs/
    ${LZMA_DIR}/src/common
    ${LZMA_DIR}/src/liblzma/api
    ${LZMA_DIR}/src/liblzma/common
    ${LZMA_DIR}/src/liblzma/lz
    ${LZMA_DIR}/src/liblzma/lzma
    ${LZMA_DIR}/src/liblzma/rangecoder
    ${LZMA_DIR}/src/liblzma/simple
    ${LZMA_DIR}/src/liblzma/delta
  )

  target_compile_definitions(liblzma PRIVATE HAVE_CONFIG_H)
  target_link_libraries(zephyr PRIVATE liblzma)

endif()
