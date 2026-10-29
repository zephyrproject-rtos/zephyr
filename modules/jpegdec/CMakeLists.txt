# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_JPEGDEC)
  # The decoder's public header is included by Zephyr headers, so its include
  # path and the non-Arduino switch apply tree-wide.
  zephyr_include_directories(${ZEPHYR_JPEGDEC_MODULE_DIR}/src)
  zephyr_compile_definitions(JPEGDEC_NO_ARDUINO)

  zephyr_library()
  zephyr_library_sources(
    ${ZEPHYR_JPEGDEC_MODULE_DIR}/src/jpegdec.c
  )
  # Portable C core only: the host SIMD paths need extra compiler flags.
  zephyr_library_compile_definitions(NO_SIMD)
  # The decoder aligns its buffers through an int64_t cast of a pointer,
  # which warns on 32-bit targets.
  zephyr_library_compile_options(-Wno-pointer-to-int-cast)
endif()
