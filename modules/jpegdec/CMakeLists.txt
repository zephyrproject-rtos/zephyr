# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_JPEGDEC)
  # Applications include zephyr_jpegdec.h; the library's own header stays
  # private to zephyr_jpegdec.c.
  zephyr_include_directories(${CMAKE_CURRENT_LIST_DIR})

  zephyr_library()
  zephyr_library_sources(zephyr_jpegdec.c)
  zephyr_library_include_directories(${ZEPHYR_JPEGDEC_MODULE_DIR}/src)
  # Use the standard C headers and build the portable C core only: the SIMD
  # paths need target-specific compiler flags.
  zephyr_library_compile_definitions(JPEGDEC_NO_ARDUINO NO_SIMD)
  # The decoder aligns its buffers through an int64_t cast of a pointer,
  # which warns on 32-bit targets, and its flash reader is used only by the
  # Arduino wrapper.
  zephyr_library_compile_options(-Wno-pointer-to-int-cast -Wno-unused-function)
endif()
