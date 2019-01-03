target_include_directories(MCUMGR INTERFACE 
    include
)

zephyr_library_sources(
    port/zephyr/src/zephyr_fs_mgmt.c
    src/fs_mgmt.c
    src/stubs.c
)
