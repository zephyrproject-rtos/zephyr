# Copyright (c) 2023 Antmicro
# SPDX-License-Identifier: Apache-2.0

add_library(EXT2 INTERFACE)
target_include_directories(EXT2 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

zephyr_library()
zephyr_library_sources(
  ext2_impl.c
  ext2_disk_access.c
  ext2_bitmap.c
  ext2_diskops.c
)
zephyr_library_sources_ifdef(CONFIG_FILE_SYSTEM ext2_ops.c)
zephyr_library_sources_ifdef(CONFIG_FILE_SYSTEM_MKFS ext2_format.c)

zephyr_library_link_libraries(EXT2)
