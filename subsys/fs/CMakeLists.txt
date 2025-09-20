# SPDX-License-Identifier: Apache-2.0

if(CONFIG_FILE_SYSTEM_LIB_LINK)
  zephyr_interface_library_named(FS)

  if(CONFIG_FILE_SYSTEM)
    zephyr_library()
    zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
    zephyr_library_sources(fs.c fs_impl.c)
    zephyr_library_sources_ifdef(CONFIG_FAT_FILESYSTEM_ELM   fat_fs.c)
    zephyr_library_sources_ifdef(CONFIG_FILE_SYSTEM_LITTLEFS littlefs_fs.c)
    zephyr_library_sources_ifdef(CONFIG_FILE_SYSTEM_SHELL    shell.c)

    zephyr_library_compile_definitions_ifdef(CONFIG_FILE_SYSTEM_LITTLEFS
                                            LFS_CONFIG=zephyr_lfs_config.h
    )
  endif()

  add_subdirectory_ifdef(CONFIG_FILE_SYSTEM_EXT2 ext2)
  add_subdirectory_ifdef(CONFIG_FUSE_CLIENT fuse_client)
  add_subdirectory_ifdef(CONFIG_FILE_SYSTEM_VIRTIOFS virtiofs)

  zephyr_library_link_libraries(FS)

  target_link_libraries_ifdef(CONFIG_FAT_FILESYSTEM_ELM   FS INTERFACE ELMFAT)
  target_link_libraries_ifdef(CONFIG_FILE_SYSTEM_LITTLEFS FS INTERFACE LITTLEFS)
  target_link_libraries_ifdef(CONFIG_FILE_SYSTEM_EXT2     FS INTERFACE EXT2)
  target_link_libraries_ifdef(CONFIG_FUSE_CLIENT          FS INTERFACE FUSE_CLIENT)
  target_link_libraries_ifdef(CONFIG_FILE_SYSTEM_VIRTIOFS FS INTERFACE VIRTIOFS)
endif()

add_subdirectory_ifdef(CONFIG_FCB  ./fcb)
add_subdirectory_ifdef(CONFIG_NVS  ./nvs)
add_subdirectory_ifdef(CONFIG_ZMS  ./zms)

if(CONFIG_FUSE_FS_ACCESS)
  zephyr_library_named(FS_FUSE)
  find_package(PkgConfig REQUIRED)
  if (CONFIG_FUSE_LIBRARY_V3)
    pkg_search_module(FUSE REQUIRED fuse3)
    target_compile_options(native_simulator INTERFACE "-DCONFIG_FUSE_LIBRARY_V3")
  else()
    pkg_search_module(FUSE REQUIRED fuse)
  endif()
  list(TRANSFORM FUSE_INCLUDE_DIRS PREPEND "-I")
  target_compile_options(native_simulator INTERFACE ${FUSE_INCLUDE_DIRS})
  list(TRANSFORM FUSE_LIBRARIES PREPEND "-l")
  target_link_options(native_simulator INTERFACE ${FUSE_LIBRARIES})
  target_sources(native_simulator INTERFACE fuse_fs_access_bottom.c)
  target_compile_options(native_simulator INTERFACE "-D_FILE_OFFSET_BITS=64")
  zephyr_library_sources(fuse_fs_access.c)
endif()
