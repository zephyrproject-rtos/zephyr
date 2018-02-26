target_include_directories(MCUMGR INTERFACE 
    include
)

zephyr_library_sources(
    cmd/stat_mgmt/port/zephyr/src/zephyr_stat_mgmt.c
    cmd/stat_mgmt/src/stat_mgmt.c
    cmd/stat_mgmt/src/stubs.c
)
