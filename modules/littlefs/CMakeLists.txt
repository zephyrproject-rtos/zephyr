if(CONFIG_FILE_SYSTEM_LITTLEFS)
  add_library(LITTLEFS INTERFACE)

  target_include_directories(LITTLEFS INTERFACE
    ${ZEPHYR_LITTLEFS_MODULE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
  )
  target_link_libraries(LITTLEFS INTERFACE zephyr_interface)
  target_compile_definitions(LITTLEFS INTERFACE LFS_CONFIG=zephyr_lfs_config.h)

  zephyr_library()
  zephyr_library_sources(
    ${ZEPHYR_LITTLEFS_MODULE_DIR}/lfs.c
    ${ZEPHYR_LITTLEFS_MODULE_DIR}/lfs_util.c
    zephyr_lfs_crc.c
  )
  zephyr_library_link_libraries(LITTLEFS)
endif()
