target_include_directories(MCUMGR INTERFACE
    include
)

zephyr_library_sources(
    port/zephyr/src/zephyr_os_mgmt.c
    src/os_mgmt.c
    src/stubs.c
)
