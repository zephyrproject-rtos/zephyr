# Copyright (c) 2022 Antmicro
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_FILE_SYSTEM_EXT2)
  add_library(EXT2 INTERFACE)
  target_include_directories(EXT2 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

  zephyr_library()
  zephyr_library_sources(
    ext2_ops.c
    ext2_mem.c
    ext2_impl.c
    ext2_blocks.c
    ext2_format.c
    ext2_flash.c
  )

  zephyr_library_link_libraries(EXT2)
endif()
