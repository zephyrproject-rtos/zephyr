add_library(NFFS INTERFACE)

target_include_directories(NFFS INTERFACE include)

zephyr_library()
zephyr_library_sources(
    src/nffs_area.c
    src/nffs_block.c
    src/nffs.c
    src/nffs_cache.c
    src/nffs_crc.c
    src/nffs_dir.c
    src/nffs_file.c
    src/nffs_flash.c
    src/nffs_format.c
    src/nffs_gc.c
    src/nffs_hash.c
    src/nffs_inode.c
    src/nffs_misc.c
    src/nffs_path.c
    src/nffs_restore.c
    src/nffs_write.c
    )
zephyr_library_link_libraries(NFFS)
target_link_libraries(NFFS INTERFACE zephyr_interface)
