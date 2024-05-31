# SPDX-License-Identifier: Apache-2.0

if(CONFIG_FILE_SYSTEM)
  zephyr_interface_library_named(FS)

  zephyr_library()
  zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
  zephyr_library_sources(fs.c fs_impl.c)
  zephyr_library_sources_ifdef(CONFIG_FAT_FILESYSTEM_ELM   fat_fs.c)
  zephyr_library_sources_ifdef(CONFIG_FILE_SYSTEM_LITTLEFS littlefs_fs.c)
  zephyr_library_sources_ifdef(CONFIG_FILE_SYSTEM_SHELL    shell.c)

  zephyr_library_compile_definitions_ifdef(CONFIG_FILE_SYSTEM_LITTLEFS
                                           LFS_CONFIG=zephyr_lfs_config.h
  )

  add_subdirectory_ifdef(CONFIG_FILE_SYSTEM_EXT2 ext2)

  zephyr_library_link_libraries(FS)

  target_link_libraries_ifdef(CONFIG_FAT_FILESYSTEM_ELM   FS INTERFACE ELMFAT)
  target_link_libraries_ifdef(CONFIG_FILE_SYSTEM_LITTLEFS FS INTERFACE LITTLEFS)
  target_link_libraries_ifdef(CONFIG_FILE_SYSTEM_EXT2     FS INTERFACE EXT2)
endif()

add_subdirectory_ifdef(CONFIG_FCB  ./fcb)
add_subdirectory_ifdef(CONFIG_NVS  ./nvs)

if(CONFIG_FUSE_FS_ACCESS)
  zephyr_library_named(FS_FUSE)
  find_package(PkgConfig REQUIRED)
  pkg_search_module(FUSE REQUIRED fuse)
  zephyr_include_directories(${FUSE_INCLUDE_DIRS})
  if (CONFIG_NATIVE_LIBRARY)
    target_link_options(native_simulator INTERFACE "-l${FUSE_LIBRARIES}")
  else()
    zephyr_link_libraries(${FUSE_LIBRARIES})
  endif()
  zephyr_library_compile_definitions(_FILE_OFFSET_BITS=64)
  zephyr_library_sources(fuse_fs_access.c)
endif()
