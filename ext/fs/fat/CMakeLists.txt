add_library(ELMFAT INTERFACE)

target_include_directories(ELMFAT INTERFACE include)

zephyr_library()
zephyr_library_sources(
    ff.c
    zfs_diskio.c
    )
zephyr_library_link_libraries(ELMFAT)
target_link_libraries(ELMFAT INTERFACE zephyr_interface)
