target_include_directories(MCUMGR INTERFACE 
    include
)

zephyr_library_sources(
    cmd/log_mgmt/port/zephyr/src/zephyr_log_mgmt.c
    cmd/log_mgmt/src/log_mgmt.c
    cmd/log_mgmt/src/stubs.c
)
