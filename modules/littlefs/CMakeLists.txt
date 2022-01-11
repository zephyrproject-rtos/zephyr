if(CONFIG_FILE_SYSTEM_LITTLEFS)
  add_library(LITTLEFS INTERFACE)

  target_include_directories(LITTLEFS INTERFACE ${ZEPHYR_LITTLEFS_MODULE_DIR})

  zephyr_library()
  zephyr_library_sources(
      ${ZEPHYR_LITTLEFS_MODULE_DIR}/lfs.c
      ${ZEPHYR_LITTLEFS_MODULE_DIR}/lfs_util.c
      zephyr_lfs_crc.c
      )
  zephyr_library_link_libraries(LITTLEFS)
  target_link_libraries(LITTLEFS INTERFACE zephyr_interface)
  zephyr_library_compile_definitions(LFS_CONFIG=zephyr_lfs_config.h)

  configure_file(zephyr_lfs_config.template
    ${PROJECT_BINARY_DIR}/include/generated/zephyr_lfs_config.h
  )
endif()
