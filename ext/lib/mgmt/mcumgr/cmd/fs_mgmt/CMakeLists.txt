target_include_directories(MCUMGR INTERFACE 
    include
)

zephyr_library_sources(
    cmd/fs_mgmt/port/zephyr/src/zephyr_fs_mgmt.c
    cmd/fs_mgmt/src/fs_mgmt.c
    cmd/fs_mgmt/src/stubs.c
)
