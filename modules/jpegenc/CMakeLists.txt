# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_JPEGENC)
  # Applications include zephyr_jpegenc.h; the library's own header stays
  # private to zephyr_jpegenc.c.
  zephyr_include_directories(${CMAKE_CURRENT_LIST_DIR})

  zephyr_library()
  zephyr_library_sources(zephyr_jpegenc.c)
  zephyr_library_include_directories(${ZEPHYR_JPEGENC_MODULE_DIR}/src)
  # Use the standard C headers instead of the Arduino ones.
  zephyr_library_compile_definitions(JPEGENC_NO_ARDUINO)
endif()
