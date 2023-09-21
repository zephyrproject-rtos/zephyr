if(CONFIG_FAT_FILESYSTEM_ELM)
  add_library(ELMFAT INTERFACE)

  target_include_directories(ELMFAT INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ZEPHYR_FATFS_MODULE_DIR}/include
  )

  target_compile_definitions(ELMFAT INTERFACE
    ZEPHYR_CONFIG_OVERRIDE=zephyr_fatfs_config.h
  )

  zephyr_library()
  zephyr_library_sources(
    ${ZEPHYR_FATFS_MODULE_DIR}/ff.c
    zfs_diskio.c
    )

  zephyr_library_sources_ifdef(CONFIG_FS_FATFS_LFN
    ${ZEPHYR_FATFS_MODULE_DIR}/option/ffunicode.c
  )

  if(DEFINED CONFIG_FS_FATFS_LFN OR DEFINED CONFIG_FS_FATFS_REENTRANT)
    zephyr_library_sources(zfs_ffsystem.c)
  endif()

  zephyr_library_link_libraries(ELMFAT)
  target_link_libraries(ELMFAT INTERFACE zephyr_interface)
endif()
