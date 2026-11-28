# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_MINIH264)
  zephyr_include_directories(${ZEPHYR_MINIH264_MODULE_DIR})
  # These select the encoder API, so every file that includes minih264e.h
  # must see the same values.
  zephyr_compile_definitions(
    H264E_ENABLE_DENOISE=0
    H264E_MAX_THREADS=0
    H264E_SVC_API=0
  )

  zephyr_library()
  zephyr_library_sources(zephyr_minih264.c)
  # The single-header implementation defines helpers for every platform.
  zephyr_library_compile_options(-Wno-unused-function)
endif()
