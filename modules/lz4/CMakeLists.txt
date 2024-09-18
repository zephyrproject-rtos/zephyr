# Copyright (c) 2020 Linumiz
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_LZ4)
  set(LZ4_DIR ${ZEPHYR_CURRENT_MODULE_DIR})

  zephyr_library()

  zephyr_include_directories(${LZ4_DIR}/lib)

  zephyr_library_compile_definitions_ifdef(CONFIG_LZ4_HEAPMODE_STACK
    LZ4_HEAPMODE=0
  )

  zephyr_library_compile_definitions_ifdef(CONFIG_LZ4_DISABLE_DYNAMIC_MEMORY_ALLOCATION
    LZ4_STATIC_LINKING_ONLY_DISABLE_MEMORY_ALLOCATION
  )

  zephyr_library_compile_definitions(
    LZ4_MEMORY_USAGE=${CONFIG_LZ4_MEMORY_USAGE}
  )

  zephyr_library_sources(
    ${LZ4_DIR}/lib/lz4.c
  )

  zephyr_library_sources_ifdef(CONFIG_LZ4_HIGH_COMPRESSION_VARIANT
    ${LZ4_DIR}/lib/lz4hc.c
  )

  zephyr_library_compile_definitions_ifdef(CONFIG_LZ4HC_HEAPMODE_STACK
    LZ4HC_HEAPMODE=0
  )

  zephyr_library_sources_ifdef(CONFIG_LZ4_XX_HASH
    ${LZ4_DIR}/lib/xxhash.c
  )

  zephyr_library_sources_ifdef(CONFIG_LZ4_FRAME_SUPPORT
    ${LZ4_DIR}/lib/lz4frame.c
  )

  zephyr_library_compile_definitions_ifdef(CONFIG_LZ4F_HEAPMODE_HEAP
    LZ4F_HEAPMODE=1
  )
endif()
