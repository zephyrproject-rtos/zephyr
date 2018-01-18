target_include_directories(MCUMGR INTERFACE 
    include
)

zephyr_library_sources(
    cmd/os_mgmt/port/zephyr/src/zephyr_os_mgmt.c
    cmd/os_mgmt/src/os_mgmt.c
    cmd/os_mgmt/src/stubs.c
)
