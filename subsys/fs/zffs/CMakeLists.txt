add_library(ZFFS INTERFACE)

target_include_directories(ZFFS INTERFACE include)

zephyr_library()
zephyr_library_sources(
	src/zffs.c
	src/zffs_area.c
	src/zffs_tree.c
	src/zffs_object.c
	src/zffs_slist.c
	src/zffs_dlist.c
	src/zffs_dir.c
	src/zffs_block.c
	src/zffs_file.c
	src/zffs_misc.c
	src/zffs_path.c
    )

zephyr_library_link_libraries(ZFFS)
target_link_libraries(ZFFS INTERFACE zephyr_interface)
