target_include_directories(MCUMGR INTERFACE
    include
)

zephyr_library_sources(
    port/zephyr/src/zephyr_img_mgmt.c
    src/img_mgmt.c
    src/img_mgmt_state.c
    src/img_mgmt_util.c
    src/stubs.c
)
