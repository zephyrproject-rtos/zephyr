if (CONFIG_FILE_SYSTEM OR CONFIG_FCB OR CONFIG_NVS)
  add_library(subsys_fs STATIC "")
  target_link_libraries(subsys_fs PRIVATE zephyr_interface)
  target_link_libraries(zephyr_app_linking INTERFACE subsys_fs)
  zephyr_set_property(TARGET subsys_fs MEMORY_SPACE kernel)
endif()

if(CONFIG_FILE_SYSTEM)
  zephyr_interface_library_named(FS)
  zephyr_link_interface_ifdef(CONFIG_FAT_FILESYSTEM_ELM ELMFAT)
  zephyr_link_interface_ifdef(CONFIG_FILE_SYSTEM_NFFS   NFFS)

  zephyr_list(SOURCES
              OUTPUT PRIVATE_SOURCES
              fs.c
              IFDEF:${CONFIG_FAT_FILESYSTEM_ELM} fat_fs.c
              IFDEF:${CONFIG_FILE_SYSTEM_NFFS} nffs_fs.c
              IFDEF:${CONFIG_FILE_SYSTEM_SHELL} shell.c
  )
  target_sources(subsys_fs PRIVATE ${PRIVATE_SOURCES})

  target_link_libraries(subsys_fs PUBLIC FS)
  target_link_libraries_ifdef(CONFIG_FAT_FILESYSTEM_ELM FS INTERFACE ELMFAT)
  target_link_libraries_ifdef(CONFIG_FILE_SYSTEM_NFFS   FS INTERFACE NFFS)
endif()


include_relative_ifdef(CONFIG_FCB fcb/CMakeLists.txt)
include_relative_ifdef(CONFIG_NVS nvs/CMakeLists.txt)
