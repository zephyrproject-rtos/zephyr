target_include_directories(MCUMGR INTERFACE
    include
)

zephyr_library_sources(
    port/zephyr/src/zephyr_stat_mgmt.c
    src/stat_mgmt.c
    src/stubs.c
)
