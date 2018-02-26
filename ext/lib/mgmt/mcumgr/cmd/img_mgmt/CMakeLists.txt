target_include_directories(MCUMGR INTERFACE 
    include
)

zephyr_library_sources(
    cmd/img_mgmt/port/zephyr/src/zephyr_img_mgmt.c
    cmd/img_mgmt/src/img_mgmt.c
    cmd/img_mgmt/src/img_mgmt_state.c
    cmd/img_mgmt/src/img_mgmt_util.c
    cmd/img_mgmt/src/stubs.c
)
